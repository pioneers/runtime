#define PY_SSIZE_T_CLEAN
#include <python3.7/Python.h>                        // For Python's C API
// #include <Python.h>                        // For Python's C API
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
#include "../logger/logger.h"              // for runtime logger

// Global variables to all functions and threads
const char* api_module = "studentapi";
PyObject *pModule, *pAPI, *pRobot, *pGamepad;
robot_desc_val_t mode = IDLE; // current robot mode
pid_t pid; // pid for mode process
int challenge_fd;  // challenge socket

// Timings for all modes
struct timespec setup_time = { 2, 0 };    // Max time allowed for setup functions
#define freq 10.0 // Minimum number of times per second the main loop should run
struct timespec main_interval = { 0, (long) ((1.0 / freq) * 1e9) };
int challenge_time = 5; // Max allowed time for running challenges in seconds

char* challenge_names[NUM_CHALLENGES] = {"reverse_digits", "list_prime_factors"};


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
 *  Resets all writeable device parameters to 0. This should be called at the end of AUTON and TELEOP.
 */
void reset_params() {
    uint32_t catalog;
    dev_id_t dev_ids[MAX_DEVICES];
    get_catalog(&catalog);
    get_device_identifiers(dev_ids);
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (catalog & (1 << i)) { // If device at index i exists
            device_t* device = get_device(dev_ids[i].type);
            if(device == NULL) {
                log_printf(ERROR, "reset_params: device at index %d with type %d is invalid\n", i, dev_ids[i].type);
                continue;
            }
            uint32_t reset_params = 0;
            param_val_t zero_params[MAX_PARAMS] = {0};
            for(int j = 0; j < device->num_params; j++) {
                if (device->params[j].write) { 
                    reset_params |= (1 << j);
                }
            }
            device_write_uid(dev_ids[i].uid, EXECUTOR, COMMAND, reset_params, zero_params);
        }
    }
}


/**
 *  Initializes the executor process. Must be the first thing called in each child subprocess
 *
 *  Input: 
 *      student_code: string representing the name of the student's Python file, without the .py
 */
void executor_init(char* student_code) {
    //initialize Python
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
    pGamepad = PyObject_CallObject(gamepad_class, NULL);
    if (pGamepad == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not instantiate Gamepad");
        exit(1);
    }
    Py_DECREF(gamepad_class);
	
	//imports the student code
    pModule = PyImport_ImportModule(student_code);
    if (pModule == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not import module: %s", student_code);
        exit(1);
    }
    
    // Insert student API into the student code
    int err = PyObject_SetAttrString(pModule, "Robot", pRobot);
    err |= PyObject_SetAttrString(pModule, "Gamepad", pGamepad);
    if (err != 0) {
        PyErr_Print();
        log_printf(ERROR, "Could not insert API into student code.");
        exit(1);
    }
    
    // Send the device subscription requests to dev_handler
    PyObject* ret = PyObject_CallMethod(pAPI, "make_device_subs", "s", student_code);
    if (ret == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not make device subscription requests");
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
 *      1: Couldn't set the mode for the Gamepad
 *      2: Python exception while running student code
 *      3: Timed out by executor
 *      4: Unable to find the given function in the student code
 */
uint8_t run_py_function(char* func_name, struct timespec* timeout, int loop, PyObject* args, PyObject** ret_value) {
    uint8_t ret = 0;
	
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

            // Set return value
            if (ret_value != NULL) {
                *ret_value = pValue;
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
                    ret = 3; // Timed out by parent process
                }
                break;
            }
            else if (ret_value == NULL) { // Decrement reference if nothing pointing at the value
                Py_DECREF(pValue);
            }
        } while(loop);
        Py_DECREF(pFunc);
    }
    else {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        log_printf(ERROR, "Cannot find function in student code: %s", func_name);
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
	
    int err = run_py_function(setup_str, &setup_time, 0, NULL, NULL);  // Run setup function once
    if (err == 0) {
        err = run_py_function(main_str, &main_interval, 1, NULL, NULL);  // Run main function on loop
        if (err != 0 && err != 3) {
            log_printf(DEBUG, "Return status of %s: %d", main_str, err);
        }
    }
    else {
        log_printf(WARN, "Won't run %s due to error in %s", main_str, setup_str);
    }
    return;    
}


/**
 *  Receives inputs from net handler, runs the coding challenges, and sends output back to net handler.
 */
void run_challenges() {
    char read_buf[CHALLENGE_LEN];
    struct sockaddr_un address;
    socklen_t addrlen = sizeof(address);
    char send_buf[NUM_CHALLENGES][CHALLENGE_LEN];
 
    for (int i = 0; i < NUM_CHALLENGES; i++) {
        int recvlen = recvfrom(challenge_fd, read_buf, CHALLENGE_LEN, 0, (struct sockaddr*) &address, &addrlen);
        if (recvlen == CHALLENGE_LEN) {
            log_printf(WARN, "UDP: Read length matches read buffer size %d", recvlen);
        }
        if (recvlen <= 0) {
            perror("recvfrom");
            log_printf(ERROR, "Socket read from challenge_fd failed");
            return;
        }
        long input = strtol(read_buf, NULL, 10);
        log_printf(DEBUG, "received inputs for %d: %ld", i, input);
        PyObject* arg = Py_BuildValue("(l)", input);
        if (arg == NULL) {
            PyErr_Print();
            log_printf(ERROR, "Couldn't decode input string into Python long for challenge %s", challenge_names[i]);
            continue;
        }
        PyObject* ret = NULL;
        if (run_py_function(challenge_names[i], NULL, 0, arg, &ret) == 3) { // Check if challenge got timed out
            strcpy(send_buf[i], "Timed out");
            for (int j = i+1; j < NUM_CHALLENGES; j++) {
                // Read rest of inputs to clear the challenge socket
                recvfrom(challenge_fd, read_buf, CHALLENGE_LEN, 0, NULL, NULL);
                // Set rest of challenge outputs to notify the TCP client
                strcpy(send_buf[j], "Timed out");
            }
            break;
        }
        Py_DECREF(arg);
        PyObject* ret_string = PyObject_Str(ret);
        Py_XDECREF(ret);
        if (ret_string == NULL) {
            PyErr_Print();
            log_printf(ERROR, "Couldn't convert return value to Python string for challenge %s", challenge_names[i]);
            continue;
        }
        int ret_len;
        const char* c_ret = PyUnicode_AsUTF8AndSize(ret_string, &ret_len);
        Py_DECREF(ret_string);
        if (ret_len >= CHALLENGE_LEN) {
            log_printf(ERROR, "Size of return string from challenge %d %s is greater than allocated %d", i, challenge_names[i], CHALLENGE_LEN);
        }
        strncpy(send_buf[i], c_ret, CHALLENGE_LEN);
        memset(read_buf, 0 , CHALLENGE_LEN);
    }

    // Send all results back to net handler
    for (int i = 0; i < NUM_CHALLENGES; i++) {
        int len = strlen(send_buf[i]) + 1;
        int sendlen = sendto(challenge_fd, send_buf[i], len, 0, (struct sockaddr*) &address, addrlen);
        if (sendlen <= 0 || sendlen != len) {
            log_printf(ERROR, "Socket send to challenge_fd failed: %s", strerror(errno));
        }
    }
    alarm(0);
}


/**
 *  Handler for killing the child mode subprocess
 */
void python_exit_handler(int signum) {
    // Cancel the Python thread by sending a TimeoutError
    log_printf(DEBUG, "sent exception 0");
    PyThreadState_SetAsyncExc((unsigned long) pthread_self(), PyExc_TimeoutError);
    log_printf(DEBUG, "sent exception");
}


/** 
 *  Kills any running subprocess. Will make the robot go into IDLE mode.
 */
void kill_subprocess() {
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
    // Reset parameters if robot was in AUTO or TELEOP. Needed for safety
    if (mode != CHALLENGE) {
        reset_params();
    }
    mode = IDLE;
}


/** 
 *  Creates a new subprocess with fork that will run the given mode using `run_mode` or `run_challenges`.
 */
pid_t start_mode_subprocess(robot_desc_val_t mode, char* student_code) {
    pid_t pid = fork();
    if (pid < 0) {
        log_printf(ERROR, "Failed to create child subprocess for mode %d: %s", mode, strerror(errno));
        return -1;
    }
    else if (pid == 0) {
        // Now in child process
        signal(SIGINT, SIG_IGN); // Disable Ctrl+C for child process
        executor_init(student_code);
        if (mode == CHALLENGE) {
            signal(SIGTERM, exit);
            signal(SIGALRM, python_exit_handler); // Interrupts any running Python function
            alarm(challenge_time); // Set timeout for challenges
            run_challenges();
            robot_desc_write(RUN_MODE, IDLE); // Will tell parent to call kill_subprocess
            while (1) {
                sleep(1); // Wait for process to be exited
            }
        }
        else {
            signal(SIGTERM, python_exit_handler);
            run_mode(mode);
            exit(0);
        }
        return pid; // Never reach this statement due to exit, needed to fix compiler warning
    }
    else {
        // Now in parent process
        return pid;
    }
}


/**
 *  Handler for keyboard interrupts SIGINT (Ctrl + C)
 */
void exit_handler(int signum) {
    log_printf(DEBUG, "Shutting down executor...");
    if (mode != IDLE) {
        kill_subprocess();
    }
    remove(CHALLENGE_SOCKET);
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
    logger_init(EXECUTOR);
    shm_init();

    char* student_code = "studentcode";
    if (argc > 1) {
        student_code = argv[1];
    }

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
                pid = start_mode_subprocess(new_mode, student_code);
                if (pid != -1) {
                    mode = new_mode;
                }
            }
        }
		usleep(100000); //throttle this thread to ~10 Hz
    }
}
