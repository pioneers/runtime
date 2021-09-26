#define PY_SSIZE_T_CLEAN
#include <arpa/inet.h>                     //for networking
#include <pthread.h>                       //for POSIX threads
#include <python3.7/Python.h>              // For Python's C API
#include <signal.h>                        // Used to handle SIGTERM, SIGINT, SIGKILL
#include <stdint.h>                        //for standard int types
#include <stdio.h>                         //for i/o
#include <stdlib.h>                        //for standard utility functions (exit, sleep)
#include <sys/types.h>                     //for sem_t and other standard system types
#include <sys/un.h>                        //for unix sockets
#include <sys/wait.h>                      //for wait functions
#include <time.h>                          // for getting time
#include <unistd.h>                        //for sleep
#include "../logger/logger.h"              // for runtime logger
#include "../runtime_util/runtime_util.h"  //for runtime constants (TODO: consider removing relative pathname in include)
#include "../shm_wrapper/shm_wrapper.h"    // Shared memory wrapper to get/send device data


// Global variables to all functions and threads
const char* api_module = "studentapi";
char* module_name;
PyObject *pModule, *pAPI, *pRobot, *pGamepad, *pKeyboard;
robot_desc_val_t mode = IDLE;  // current robot mode
pid_t pid;                     // pid for mode process

// Timings for all modes
struct timespec setup_time = {2, 0};  // Max time allowed for setup functions
#define MIN_FREQ 10.0                 // Minimum number of times per second the main loop should run
struct timespec main_interval = {0, (long) ((1.0 / MIN_FREQ) * 1e9)};
#define MAX_FREQ 10000.0                     // Maximum number of times per second the Python function should run
uint64_t min_time = (1.0 / MAX_FREQ) * 1e9;  // Minimum time in nanoseconds that the Python function should take


/**
 *  Returns the appropriate string representation from the given mode, or NULL if the mode is invalid.
 */
static char* get_mode_str(robot_desc_val_t mode) {
    if (mode == AUTO) {
        return "autonomous";
    } else if (mode == TELEOP) {
        return "teleop";
    } else if (mode == IDLE) {
        return "idle";
    }
    log_printf(ERROR, "Run mode %d is invalid", mode);
    return NULL;
}


/**
 *  Resets relevant parameters to default values. This should be called at the end of AUTON and TELEOP.
 */
static void reset_params() {
    uint32_t catalog;
    dev_id_t dev_ids[MAX_DEVICES];
    get_catalog(&catalog);
    get_device_identifiers(dev_ids);
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (catalog & (1 << i)) {  // If device at index i exists
            device_t* device = get_device(dev_ids[i].type);
            if (device == NULL) {
                log_printf(ERROR, "reset_params: device at index %d with type %d is invalid\n", i, dev_ids[i].type);
                continue;
            }
            uint32_t params_to_reset = 0;
            param_val_t zero_params[MAX_PARAMS] = {0};  // By default we reset to 0

            // reset KoalaBear velocity_a, velocity_b params to 0
            if (strcmp(device->name, "KoalaBear") == 0) {
                for (int j = 0; j < device->num_params; j++) {
                    if (strcmp(device->params[j].name, "velocity_a") == 0) {
                        params_to_reset |= (1 << j);
                    } else if (strcmp(device->params[j].name, "velocity_b") == 0) {
                        params_to_reset |= (1 << j);
                    }
                }
                device_write_uid(dev_ids[i].uid, EXECUTOR, COMMAND, params_to_reset, zero_params);
            }
            params_to_reset = 0;

            // TODO: if more params for more devices need to be reset, follow construction above ^
        }
    }
}


/**
 *  Initializes the executor process. Must be the first thing called in each child subprocess
 *
 *  Input:
 *      student_code: string representing the name of the student's Python file, without the .py
 */
static void executor_init(char* student_code) {
    // initialize Python
    Py_Initialize();
    PyEval_InitThreads();
    // Need this so that the Python interpreter sees the Python files in this directory
    PyRun_SimpleString("import sys;sys.path.insert(0, '.')");

    //imports the Cython student API
    pAPI = PyImport_ImportModule(api_module);
    if (pAPI == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not import API module");
        exit(1);
    }

    // imports the student code
    module_name = student_code;
    pModule = PyImport_ImportModule(module_name);
    if (pModule == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not import student code file: %s", module_name);
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
    pGamepad = PyObject_CallObject(gamepad_class, NULL);
    if (pGamepad == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not instantiate Gamepad");
        exit(1);
    }
    Py_DECREF(gamepad_class);

    //checks to make sure there is a Keyboard class, then instantiates it
    PyObject* keyboard_class = PyObject_GetAttrString(pAPI, "Keyboard");
    if (keyboard_class == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not find Keyboard class");
        exit(1);
    }
    pKeyboard = PyObject_CallObject(keyboard_class, NULL);
    if (pKeyboard == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not instantiate Keyboard");
        exit(1);
    }
    Py_DECREF(keyboard_class);

    // Insert student API into the student code
    int err = PyObject_SetAttrString(pModule, "Robot", pRobot);
    err |= PyObject_SetAttrString(pModule, "Gamepad", pGamepad);
    err |= PyObject_SetAttrString(pModule, "Keyboard", pKeyboard);
    if (err != 0) {
        PyErr_Print();
        log_printf(ERROR, "Could not insert API into student code.");
        exit(1);
    }
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
 *      1: Python exception in student actions
 *      2: Python exception while running student code
 *      3: Timed out by executor
 *      4: Unable to find the given function in the student code
 */
static uint8_t run_py_function(const char* func_name, struct timespec* timeout, int loop, PyObject* args, PyObject** py_ret) {
    uint8_t ret = 0;

    //retrieve the Python function from the student code
    PyObject* pFunc = PyObject_GetAttrString(pModule, func_name);
    PyObject* pValue = NULL;
    if (pFunc && PyCallable_Check(pFunc)) {
        struct timespec start, end;
        uint64_t time, max_time = 0;
        if (timeout != NULL) {
            max_time = timeout->tv_sec * 1e9 + timeout->tv_nsec;
        }

        do {
            clock_gettime(CLOCK_MONOTONIC, &start);
            pValue = PyObject_CallObject(pFunc, args);  // make call to Python function
            clock_gettime(CLOCK_MONOTONIC, &end);

            //if the time the Python function took was greater than max_time, warn that it's taking too long
            time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
            if (timeout != NULL && time > max_time) {
                log_printf(WARN, "Function %s is taking longer than %lu milliseconds, indicating a loop or sleep in the code. You probably forgot to put a Robot.sleep call into a robot action instead of a regular function.", func_name, (long) (max_time / 1e6));
            }
            //if the time the Python function took was less than min_time, sleep to slow down execution
            if (time < min_time) {
                usleep((min_time - time) / 1000);  // Need to convert nanoseconds to microseconds
            }

            // Set return value
            if (py_ret != NULL) {
                Py_XDECREF(*py_ret);  // Decrement previous reference, if it exists
                *py_ret = pValue;
            } else {
                Py_XDECREF(pValue);
            }

            //catch execution error
            if (pValue == NULL) {
                if (!PyErr_ExceptionMatches(PyExc_TimeoutError)) {
                    PyErr_Print();
                    log_printf(ERROR, "Python function %s call failed", func_name);
                    ret = 2;
                } else {
                    ret = 3;  // Timed out by parent process
                }
                break;
            } else if (mode == AUTO || mode == TELEOP) {
                // Need to check if error occurred in action thread
                PyObject* event = PyObject_GetAttrString(pRobot, "error_event");
                if (event == NULL) {
                    PyErr_Print();
                    log_printf(ERROR, "Could not get error_event from Robot instance");
                    exit(2);
                }
                PyObject* event_set = PyObject_CallMethod(event, "is_set", NULL);
                if (event_set == NULL) {
                    if (!PyErr_ExceptionMatches(PyExc_TimeoutError)) {
                        PyErr_Print();
                        log_printf(DEBUG, "Could not get if error is set from error_event");
                        exit(2);
                    } else {
                        ret = 3;  // Timed out by parent process
                    }
                    break;
                } else if (PyObject_IsTrue(event_set) == 1) {
                    log_printf(ERROR, "Stopping %s due to error in action", func_name);
                    ret = 1;
                    break;
                }
            }
        } while (loop);
        Py_DECREF(pFunc);
    } else {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        log_printf(ERROR, "Cannot find function %s in code file %s", func_name, module_name);
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
static void run_mode(robot_desc_val_t mode) {
    //Set up the arguments to the threads that will run the setup and main threads
    char* mode_str = get_mode_str(mode);
    char setup_str[20], main_str[20];
    sprintf(setup_str, "%s_setup", mode_str);
    sprintf(main_str, "%s_main", mode_str);

    int err = run_py_function(setup_str, &setup_time, 0, NULL, NULL);  // Run setup function once
    if (err == 0) {
        err = run_py_function(main_str, &main_interval, 1, NULL, NULL);  // Run main function on loop
    } else {
        log_printf(WARN, "Won't run %s due to error %d in %s", main_str, err, setup_str);
    }
    return;
}


/**
 *  Handler for killing the child mode subprocess
 */
static void python_exit_handler(int signum) {
    exit(0);
    // Cancel the Python thread by sending a TimeoutError
    // log_printf(DEBUG, "cancelling Python function");
    // PyGILState_STATE gstate = PyGILState_Ensure();
    //     PyObject* ret = PyObject_CallMethod(event, "set", NULL);
    //     Py_DECREF(event);
    //     if (ret == NULL) {
    //         PyErr_Print();
    //         log_printf(ERROR, "Could not set sleep_event to True");
    //         exit(2);
    //     }
    //     Py_DECREF(ret);
    // }
    // PyThreadState_SetAsyncExc((unsigned long) pthread_self(), PyExc_TimeoutError);
    // PyGILState_Release(gstate);
}


/**
 *  Kills any running subprocess. Will make the robot go into IDLE mode.
 */
static void kill_subprocess() {
    if (kill(pid, SIGTERM) != 0) {
        log_printf(ERROR, "Kill signal not sent: %s", strerror(errno));
    }
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        log_printf(ERROR, "Wait failed for pid %d: %s", pid, strerror(errno));
    }
    if (!WIFEXITED(status)) {
        log_printf(ERROR, "Error when shutting down execution of mode %d", mode);
    }
    if (WIFSIGNALED(status)) {
        log_printf(ERROR, "killed by signal %d\n", WTERMSIG(status));
    }
    reset_params();
    mode = IDLE;
}


/**
 *  Creates a new subprocess with fork that will run the given mode using `run_mode`
 */
static pid_t start_mode_subprocess(char* student_code) {
    pid_t pid = fork();
    if (pid < 0) {
        log_printf(ERROR, "Failed to create child subprocess for mode %d: %s", mode, strerror(errno));
        return -1;
    } else if (pid == 0) {
        // Now in child process
        signal(SIGINT, SIG_IGN);  // Disable Ctrl+C for child process
        executor_init(student_code);
        signal(SIGTERM, python_exit_handler);  // Set handler for killing subprocess
        run_mode(mode);
        exit(0);
        return pid;  // Never reach this statement due to exit, needed to fix compiler warning
    } else {
        // Now in parent process
        return pid;
    }
}


/**
 *  Handler for keyboard interrupts SIGINT (Ctrl + C)
 */
static void exit_handler(int signum) {
    log_printf(INFO, "Shutting down executor...");
    if (mode != IDLE) {
        kill_subprocess();
    }
    exit(0);
}


/**
 *  Main bootloader that calls `run_mode` in a separate process with the correct mode. Ensures any previously running
 *  process is terminated first.
 *
 *  Behavior: This is a blocking function and will begin handling the run mode forever until a SIGINT.
 *
 *  CLI Args:
 *      1: name of the Python file that contains the student auton and teleop functions, without the '.py', Default is "studentcode"
 *
 */
int main(int argc, char* argv[]) {
    signal(SIGINT, exit_handler);
    logger_init(EXECUTOR);
    shm_init();

    char* student_code = "studentcode";
    if (argc > 1) {
        student_code = argv[1];
    }
    log_printf(INFO, "Executor initialized");

    robot_desc_val_t new_mode = IDLE;
    // Main loop that checks for new run mode in shared memory from the network handler
    while (1) {
        new_mode = robot_desc_read(RUN_MODE);
        // If we receive a new mode, cancel the previous mode and start the new one
        if (new_mode != mode) {
            if (mode != IDLE) {
                kill_subprocess();
            }
            if (new_mode != IDLE) {
                mode = new_mode;
                pid = start_mode_subprocess(student_code);
                if (pid == -1) {
                    mode = IDLE;
                }
            }
        }
        usleep(100000);  //throttle this thread to ~10 Hz
    }
}
