#define PY_SSIZE_T_CLEAN
#include <Python.h>                        // For Python's C API
#include <stdio.h>                         //for i/o
#include <stdlib.h>                        //for standard utility functions (exit, sleep)
#include <sys/types.h>                     //for sem_t and other standard system types
#include <sys/wait.h>                      //for wait functions
#include <sys/un.h>                        //for unix sockets
#include <arpa/inet.h>                     //for networking
#include <stdint.h>                        //for standard int types
#include <unistd.h>                        //for sleep
#include <pthread.h>                       //for POSIX threads
#include <signal.h>                        // Used to handle SIGTERM, SIGINT, SIGKILL
#include <time.h>                          // for getting time
#include "../runtime_util/runtime_util.h"  //for runtime constants (TODO: consider removing relative pathname in include)
#include "../shm_wrapper/shm_wrapper.h"    // Shared memory wrapper to get/send device data
#include "../shm_wrapper_aux/shm_wrapper_aux.h" // Shared memory wrapper for robot state
#include "../logger/logger.h"              // for runtime logger
#include "../net_handler/pbc_gen/text.pb-c.h" // for Text protobuf

// Global variables to all functions and threads
const char* api_module = "studentapi";
char* student_module;
PyObject *pModule, *pAPI, *pRobot, *pGamepad;
robot_desc_val_t mode = IDLE; // current robot mode
pid_t pid; // pid for mode process
int challenge_fd;  // challenge socket

// Timings for all modes
struct timespec setup_time = { 2, 0 };    // Max time allowed for setup functions
#define freq 10.0 // Minimum number of times per second the main loop should run
struct timespec main_interval = { 0, (long) ((1.0 / freq) * 1e9) };
int challenge_time = 5; // Max allowed time for running challenges in seconds

char* challenge_names[NUM_CHALLENGES] = {"reverse_digits"};


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
	log_printf(DEBUG, "Shutting down executor...");
    
    // Py_XDECREF(pAPI);
    // Py_XDECREF(pGamepad);
    // Py_XDECREF(pRobot);
    // Py_XDECREF(pModule);
    // Py_FinalizeEx();
    log_printf(DEBUG, "Trying to shut down aux SHM");
    shm_aux_stop(EXECUTOR);
    log_printf(DEBUG, "Aux SHM stopped");
    shm_stop(EXECUTOR);
    log_printf(DEBUG, "SHM stopped");
    logger_stop();
    exit(2);
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
        exit(1);
    }
	
	//checks to make sure there is a Robot class, then instantiates it
    PyObject* robot_class = PyObject_GetAttrString(pAPI, "Robot");
    if (robot_class == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not find Robot class");
        exit(1);
    }
    pRobot = PyObject_CallObject(robot_class, NULL);
    if (pRobot == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not instantiate Robot");
        exit(1);
    }
    Py_DECREF(robot_class);
	
	//checks to make sure there is a Gamepad class, then instantiates it
    PyObject* gamepad_class = PyObject_GetAttrString(pAPI, "Gamepad");
    if (gamepad_class == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not find Gamepad class");
        exit(1);
    }
    pGamepad = PyObject_CallFunction(gamepad_class, "s", "idle");
    if (pGamepad == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not instantiate Gamepad");
        exit(1);
    }
    Py_DECREF(gamepad_class);
	
	//imports the student code
    pModule = PyImport_ImportModule(student_module);
    if (pModule == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not import module: %s", student_module);
        exit(1);
    }
    
    int err = PyObject_SetAttrString(pModule, "Robot", pRobot);
    err |= PyObject_SetAttrString(pModule, "Gamepad", pGamepad);
    if (err != 0) {
        PyErr_Print();
        log_printf(ERROR, "Could not insert API into student code.");
        exit(1);
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
uint8_t run_py_function(char* func_name, char* mode, struct timespec* timeout, int loop, PyObject* args, PyObject** ret_value) {
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
        if (timeout != NULL) {
            max_time = timeout->tv_sec * 1e9 + timeout->tv_nsec;
        }

        do {
            clock_gettime(CLOCK_MONOTONIC, &start);
            pValue = PyObject_CallObject(pFunc, args); // make call to Python function
            clock_gettime(CLOCK_MONOTONIC, &end);

			//if the time the Python function took was greater than max_time, warn that it's taking too long
            time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
            if (timeout != NULL && time > max_time) {
                log_printf(WARN, "Function %s is taking longer than %lu milliseconds, indicating a loop in the code.", func_name, (long) (max_time / 1e6));
            }

			//catch execution error
            if (pValue == NULL) {
                *ret_value = NULL;
                PyObject* error = PyErr_Occurred();
                if (!PyErr_GivenExceptionMatches(error, PyExc_TimeoutError)) {
                    PyErr_Print();
                    log_printf(ERROR, "Python function %s call failed", func_name);
                    ret = 2;
                }
                else {
                    log_printf(DEBUG, "Timed out by supervisor");
                    ret = 3; // Timed out by supervisor process
                }
                break;
            }
            else if (ret_value != NULL) {
                log_printf(DEBUG, "I'm here");
                *ret_value = pValue;
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
    log_printf(DEBUG, "Returning from run_py_function");
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
	
    int err = run_py_function(setup_str, mode_str, &setup_time, 0, NULL, NULL);  // Run setup function once
    if (err == 0) {
        err = run_py_function(main_str, mode_str, &main_interval, 1, NULL, NULL);  // Run main function on loop
        if (err != 0) {
            log_printf(DEBUG, "Return status of %s: %d", main_str, err);
        }
    }
    else {
        log_printf(WARN, "Won't run %s due to error in %s", main_str, setup_str);
    }
    return;    
}


void run_challenges() {
    int size = 4096;
    uint8_t read_buf[size];
    struct sockaddr_un address;
    socklen_t addrlen = sizeof(address);
    int recvlen = recvfrom(challenge_fd, read_buf, size, 0, (struct sockaddr*) &address, &addrlen);
    if (recvlen == size) {
	    log_printf(WARN, "UDP: Read length matches read buffer size %d", recvlen);
	}
    if (recvlen <= 0) {
        perror("recvfrom");
        log_printf(ERROR, "Socket read from challenge_fd failed");
        return;
    }

    Text* challenge_inputs = text__unpack(NULL, recvlen, read_buf);
    if (challenge_inputs == NULL) {
        log_printf(ERROR, "Failed to unpack challenge inputs");
        return;
    }
    if (challenge_inputs->n_payload != NUM_CHALLENGES) {
        log_printf(ERROR, "Number of inputs sent doesn't match number of challenges");
        return;
    }

    Text challenge_outputs = TEXT__INIT;
    challenge_outputs.n_payload = NUM_CHALLENGES;
    challenge_outputs.payload = malloc(sizeof(char*) * NUM_CHALLENGES);
    int ret_size = 32;

    for (int i = 0; i < NUM_CHALLENGES; i++) {
        challenge_outputs.payload[i] = malloc(sizeof(char) * ret_size);
        // PyObject* arg = PyLong_FromString(challenge_inputs->payload[i], NULL, 10);
        long input = atoi(challenge_inputs->payload[i]);
        PyObject* arg = Py_BuildValue("(l)", input);
        if (arg == NULL) {
            PyErr_Print();
            log_printf(ERROR, "Couldn't decode input string into Python long for challenge %s", challenge_names[i]);
            continue;
        }
        PyObject* ret;
        run_py_function(challenge_names[i], "challenge", NULL, 0, arg, &ret);
        Py_DECREF(arg);
        long return_val = PyLong_AsLong(ret);
        Py_DECREF(ret);
        if (return_val == -1 && PyErr_Occurred()) {
            PyErr_Print();
            log_printf(ERROR, "Couldn't decode return int into Python int for challenge %s", challenge_names[i]);
            continue;
        }
        sprintf(challenge_outputs.payload[i], "%ld", return_val);
    }
    text__free_unpacked(challenge_inputs, NULL);

    int pb_len = text__get_packed_size(&challenge_outputs);
    uint8_t send_buf[pb_len];
    text__pack(&challenge_outputs, send_buf);

    int sendlen = sendto(challenge_fd, send_buf, pb_len, 0, (struct sockaddr*) &address, addrlen);
    if (sendlen <= 0) {
        perror("sendto");
        log_printf(ERROR, "Socket send to challenge_fd failed");
    }
    alarm(0);
}


/** 
 *  Creates a new subprocess with fork that will run the given mode using `run_mode`.
 */
pid_t start_mode_subprocess(robot_desc_val_t mode) {
    int pipe_fd[2];
    if (pipe(pipe_fd) != 0) {
        log_printf(ERROR, "Pipe failed: %s", strerror(errno));
        return NULL;
    }
    pid_t pid = fork();
    if (pid < 0) {
        log_printf(ERROR, "Failed to create child subprocess for mode %d: %s", mode, strerror(errno));
        return -1;
    }
    else if (pid == 0) {
        // Now in child process
        // atexit(executor_stop);
        signal(SIGALRM, child_exit_handler);
        executor_init("studentcode"); // Default name of student code file 
        printf("In child process stdout \n");
        fprintf(stderr, "sending to child stderr \n");
        if (mode == CHALLENGE) {
            alarm(challenge_time);
            run_challenges();
        }
        else {
            run_mode(mode);
            printf("Finished running mode %d\n", mode);
        }
        executor_stop(); 
        // exit(2);
        return pid; // Never reach this statement due to exit
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
    int sig;
    if (mode == CHALLENGE) {
        sig = SIGALRM;
    }
    else {
        sig = SIGALRM;
    }
    if (kill(pid, SIGALRM) != 0) {
        log_printf(ERROR, "Kill signal not sent: %s", strerror(errno));
    } 
    else {
        log_printf(DEBUG, "Sent kill signal to child process");
    }
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        log_printf(ERROR, "Wait failed for pid %d: %s", pid, strerror(errno));
    }
    else if (!WIFEXITED(status)) {
        log_printf(ERROR, "Error when shutting down execution of mode %d", mode);
    }
    else if (WIFSIGNALED(status)) {
        log_printf(ERROR, "killed by signal %d\n", WTERMSIG(status));
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
    remove(CHALLENGE_SOCKET);
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

    struct sockaddr_un my_addr = {AF_UNIX, CHALLENGE_SOCKET};    //for holding IP addresses (IPv4)
	//create the socket
	if ((challenge_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
        log_printf(FATAL, "could not create challenge socket");
		return 1;
	}

	if (bind(challenge_fd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_un)) < 0) {
        log_printf(FATAL, "challenge socket bind failed: %s", strerror(errno));
		return 1;
	}

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
