#define PY_SSIZE_T_CLEAN
#include <Python.h>                        // For Python's C API
#include <stdio.h>                         //for i/o
#include <stdlib.h>                        //for standard utility functions (exit, sleep)
#include <sys/types.h>                     //for sem_t and other standard system types
#include <sys/wait.h>                      //for wait functions
#include <stdint.h>                        //for standard int types
#include <unistd.h>                        //for sleep
#include <pthread.h>                       //for POSIX threads
#include <signal.h>                        // Used to handle SIGTERM, SIGINT, SIGKILL
#include <time.h>                          // for getting time
#include "../runtime_util/runtime_util.h"  //for runtime constants (TODO: consider removing relative pathname in include)
#include "../shm_wrapper/shm_wrapper.h"    // Shared memory wrapper to get/send device data
#include "../shm_wrapper_aux/shm_wrapper_aux.h" // Shared memory wrapper for robot state
#include "../logger/logger.h"              // for runtime logger

// Global variables to all functions and threads
const char* api_module = "studentapi";
char* student_module;
PyObject *pModule, *pAPI, *pRobot, *pGamepad;
robot_desc_val_t mode = IDLE;
pid_t pid;

// Timings for all modes
struct timespec setup_time = { 2, 0 };    // Max time allowed for setup functions
#define freq 10.0 // Minimum number of times per second the main loop should run
struct timespec main_interval = { 0, (long) ((1.0 / freq) * 1e9) };


/**
 *  Returns the appropriate string representation from the given mode.
 */
char* get_mode_str(robot_desc_val_t mode) {
    if (mode == AUTO) {
        return "autonomous";
    }
    else if (mode == TELEOP) {
        return "teleop";
    }
    else if (mode == IDLE) {
        return "idle";
    }
    log_printf(ERROR, "Run mode %d is invalid", mode);
    return NULL;
}


/**
 *  Closes all executor processes and exits cleanly.
 */
void executor_stop() {
	printf("\nShutting down executor...\n");
    
    // Py_XDECREF(pAPI);
    // Py_XDECREF(pGamepad);
    // Py_XDECREF(pRobot);
    // Py_XDECREF(pModule);
    // Py_FinalizeEx();
    
    shm_aux_stop(EXECUTOR);
    log_printf(DEBUG, "Aux SHM stopped");
    shm_stop(EXECUTOR);
    log_printf(DEBUG, "SHM stopped");
    logger_stop();
	exit(0);
    return;
}


/**
 *  Handler for killing the child subprocess
 */
void child_exit_handler(int signum) {
    // Cancel the Python thread by sending a TimeoutError
    PyThreadState_SetAsyncExc((unsigned long) pthread_self(), PyExc_TimeoutError);
    log_printf(DEBUG, "Sent exception");
}


/**
 *  Initializes the executor process. Must be the first thing called in each child subprocess
 *
 *  Input: 
 *      student_code: string representing the name of the student's Python file, without the .py
 */
void executor_init(char* student_code) {
    //initialize
	logger_init(EXECUTOR);
    shm_aux_init(EXECUTOR);
    log_printf(DEBUG, "Aux SHM initialized");
    shm_init(EXECUTOR);
    log_printf(DEBUG, "SHM intialized");
    student_module = student_code;
    
    Py_Initialize();
    // Need this so that the Python interpreter sees the Python files in this directory
    PyRun_SimpleString("import sys;sys.path.insert(0, '.')");
	
	//imports the Cython student API
    pAPI = PyImport_ImportModule(api_module);
    if (pAPI == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not import API module");
        executor_stop();
    }
	
	//checks to make sure there is a Robot class, then instantiates it
    PyObject* robot_class = PyObject_GetAttrString(pAPI, "Robot");
    if (robot_class == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not find Robot class");
        executor_stop();
    }
    pRobot = PyObject_CallObject(robot_class, NULL);
    if (pRobot == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not instantiate Robot");
        executor_stop();
    }
    Py_DECREF(robot_class);
	
	//checks to make sure there is a Gamepad class, then instantiates it
    PyObject* gamepad_class = PyObject_GetAttrString(pAPI, "Gamepad");
    if (gamepad_class == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not find Gamepad class");
        executor_stop();
    }
    pGamepad = PyObject_CallFunction(gamepad_class, "s", "idle");
    if (pGamepad == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not instantiate Gamepad");
        executor_stop();
    }
    Py_DECREF(gamepad_class);
	
	//imports the student code
    pModule = PyImport_ImportModule(student_module);
    if (pModule == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not import module: %s", student_module);
        executor_stop();
    }
    
    int err = PyObject_SetAttrString(pModule, "Robot", pRobot);
    err |= PyObject_SetAttrString(pModule, "Gamepad", pGamepad);
    if (err != 0) {
        PyErr_Print();
        log_printf(ERROR, "Could not insert API into student code.");
        executor_stop();
    }

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
}


/**
 *  Runs the Python function specified in the arguments.
 * 
 *  Behavior: If loop = 0, this will block the calling thread for the length of
 *  the Python function call. If loop is nonzero, this will block the calling thread forever.
 *  This function should be run in a separate thread.
 * 
 *  Inputs:
 *      Necessary fields:
 *          func_name: string of function name to run in the student code
 *          mode: string of the current mode
 *          loop: boolean for whether the Python function should be called in a while loop forever
 *      Optional fields:
 *          timeout: max length of execution time before a warning is issued for the function call
 *
 *  Returns: error code of function
 *      0: Exited cleanly
 *      1: Couldn't set the mode for the Gamepad
 *      2: Python exception while running student code
 *      3: Timed out by executor
 *      4: Unable to find the given function in the student code
 */
uint8_t run_py_function(char* func_name, char* mode, struct timespec* timeout, int loop) {
    uint8_t ret = 0;
	
	//assign the run mode to the Gamepad object
    PyObject *pMode = PyUnicode_FromString(mode);
    int err = PyObject_SetAttrString(pGamepad, "mode", pMode);
    Py_DECREF(pMode);
    if (err != 0) {
        PyErr_Print();
        log_printf(ERROR, "Couldn't assign mode for Gamepad while trying to run %s", func_name);
        return 1;
    }
	
    //retrieve the Python function from the student code
    PyObject *pFunc = PyObject_GetAttrString(pModule, func_name);
    PyObject *pValue = NULL;
    if (pFunc && PyCallable_Check(pFunc)) {
        struct timespec start, end;
        uint64_t time, max_time = 0;
        if (timeout) {
            max_time = timeout->tv_sec * 1e9 + timeout->tv_nsec;
        }

        do {
            clock_gettime(CLOCK_MONOTONIC, &start);
            pValue = PyObject_CallObject(pFunc, NULL); // make call to Python function
            clock_gettime(CLOCK_MONOTONIC, &end);

			//if the time the Python function took was greater than max_time, warn that it's taking too long
            time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
            if (timeout && time > max_time) {
                log_printf(WARN, "Function %s is taking longer than %lu milliseconds, indicating a loop in the code.", func_name, (long) (max_time / 1e6));
            }
			//catch execution error
            if (pValue == NULL) {
                PyObject* error = PyErr_Occurred();
                if (!PyErr_GivenExceptionMatches(error, PyExc_TimeoutError)) {
                    PyErr_Print();
                    log_printf(ERROR, "Python function %s call failed", func_name);
                    ret = 2;
                }
                else {
                    ret = 3;
                }
                break;
            }
            else {
                Py_DECREF(pValue);
            }
        } while(loop);
        Py_DECREF(pFunc);
    }
    else {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        log_printf(ERROR, "Cannot find function in student code: %s\n", func_name);
        ret = 4;
    }
    return ret;
}


/**
 *  Begins the given game mode and calls setup and main appropriately. Will run main forever.
 * 
 *  Behavior: This is a blocking function and will block the calling thread forever.
 *  This should only be run as a separate thread.
 * 
 *  Inputs:
 *      args: string of the mode to start running
 */
void run_mode(robot_desc_val_t mode) {
	//Set up the arguments to the threads that will run the setup and main threads
    char* mode_str = get_mode_str(mode);
    char setup_str[20], main_str[20];
    sprintf(setup_str, "%s_setup", mode_str);
    sprintf(main_str, "%s_main", mode_str);
	
    int err = run_py_function(setup_str, mode_str, &setup_time, 0);  // Run setup function once
    if (err == 0) {
        run_py_function(main_str, mode_str, &main_interval, 1);  // Run main function on loop
    }
    else {
        log_printf(WARN, "Won't run %s due to error in %s", main_str, setup_str);
    }
    return;    
}


/** 
 *  Creates a new subprocess with fork that will run the given mode using `run_mode`.
 */
pid_t start_mode_subprocess(robot_desc_val_t mode) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        log_printf(ERROR, "Failed to create child subprocess for mode %d", mode);
        return -1;
    }
    else if (pid == 0) {
        // Now in child process
        signal(SIGALRM, child_exit_handler);
        executor_init("studentcode"); // Default name of student code file 
        run_mode(mode);
        executor_stop();
        return pid; // Never reach this statement due to exit in executor_stop
    }
    else {
        // Now in parent process
        return pid;
    }
}


/** 
 *  Kills any running subprocess.
 */
void kill_subprocess() {
    kill(pid, SIGALRM);
    log_printf(DEBUG, "Sent kill signal to child process");
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status)) {
        log_printf(ERROR, "Error when shutting down execution of mode %d", mode);
    }
    mode = IDLE;
}


/**
 *  Handler for keyboard interrupts SIGINT (Ctrl + C)
 */
void exit_handler(int signum) {
    log_printf(DEBUG, "Shutting down supervisor...");
    if (mode != IDLE) {
        kill_subprocess();
    }
    shm_aux_stop(SUPERVISOR);
    logger_stop();
    exit(0);
}


/**
 *  Main bootloader that calls `run_mode` in a separate process with the correct mode. Ensures any previously running
 *  process is terminated first.
 * 
 *  Behavior: This is a blocking function and will begin handling the run mode forever until a SIGINT.
 */
int main(int argc, char* argv[]) {
    signal(SIGINT, exit_handler);
    logger_init(SUPERVISOR);
    log_printf(DEBUG, "Logger initialized");
    shm_aux_init(SUPERVISOR);
    log_printf(DEBUG, "Aux SHM started");

    robot_desc_val_t new_mode = IDLE;

    // Main loop that checks for new run mode in shared memory from the network handler
    while(1) {
        new_mode = robot_desc_read(RUN_MODE);
        // If we receive a new mode, cancel the previous mode and start the new one
        if (new_mode != mode) {
            if (mode != IDLE) {
                kill_subprocess();
            }
            log_printf(DEBUG, "New mode %d", new_mode);
            if (new_mode != IDLE) {
                pid = start_mode_subprocess(new_mode);
                if (pid != -1) {
                    mode = new_mode;
                }
            }
        }
    }
}
