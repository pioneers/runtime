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
#include "../net_handler/net_util.h"       // For Text protobuf message and buffer utilities
#include "../runtime_util/runtime_util.h"  //for runtime constants (TODO: consider removing relative pathname in include)
#include "../shm_wrapper/shm_wrapper.h"    // Shared memory wrapper to get/send device data

#define DEBUG_SIGNAL 15  // Signal to log as DEBUG instead of ERROR

// Global variables to all functions and threads
const char* api_module = "studentapi";
char* module_name;
PyObject *pModule, *pAPI, *pRobot, *pGamepad, *pKeyboard;
robot_desc_val_t mode = IDLE;  // current robot mode
pid_t pid;                     // pid for mode process
int challenge_fd;              // challenge socket

// Timings for all modes
struct timespec setup_time = {2, 0};  // Max time allowed for setup functions
#define MIN_FREQ 10.0                 // Minimum number of times per second the main loop should run
struct timespec main_interval = {0, (long) ((1.0 / MIN_FREQ) * 1e9)};
#define MAX_FREQ 10000.0                     // Maximum number of times per second the Python function should run
uint64_t min_time = (1.0 / MAX_FREQ) * 1e9;  // Minimum time in nanoseconds that the Python function should take
#define CHALLENGE_TIME 5                     // Max allowed time for running all challenges in seconds


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

    // imports the student code
    module_name = student_code;
    pModule = PyImport_ImportModule(module_name);
    if (pModule == NULL) {
        PyErr_Print();
        log_printf(ERROR, "Could not import student code file: %s", module_name);
        exit(1);
    }

    // CHALLENGE mode doesn't need the robot API
    if (mode == CHALLENGE) {
        return;
    }

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
            } else if (mode != CHALLENGE) {
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
 *  Receives inputs from net handler, runs the coding challenges, and sends output back to net handler.
 */
static void run_challenges() {
    PyObject* challenge_list = PyObject_GetAttrString(pModule, "CHALLENGES");
    if (challenge_list == NULL) {
        PyErr_Print();
        log_printf(ERROR, "run_challenges: cannot find the CHALLENGES list in challenges file %s", module_name);
        return;
    }
    uint8_t num_challenges = PyList_Size(challenge_list);
    if (PyErr_Occurred()) {
        PyErr_Print();
        log_printf(ERROR, "run_challenges: cannot get length of CHALLENGES object, not a Python list. %d", num_challenges);
        return;
    }

    // Receive challenge inputs from net_handler
    struct sockaddr_un address;
    socklen_t addrlen = sizeof(address);
    int buf_size = 256;
    uint8_t read_buf[buf_size];

    int recv_len = recvfrom(challenge_fd, read_buf, buf_size, 0, (struct sockaddr*) &address, &addrlen);
    if (recv_len == buf_size) {
        log_printf(WARN, "run_challenges: Read length matches read buffer size %d", recv_len);
    }
    if (recv_len <= 0) {
        log_printf(ERROR, "run_challenges: socket read from challenge_fd failed: recvfrom %s", strerror(errno));
        return;
    }
    Text* inputs = text__unpack(NULL, recv_len, read_buf);
    if (inputs == NULL) {
        log_printf(ERROR, "run_challenges: failed to unpack challenge input protobuf");
        return;
    }
    if (num_challenges != inputs->n_payload) {
        log_printf(ERROR, "run_challenges: number of challenge names %d is not equal to number of inputs %d", num_challenges, inputs->n_payload);
        return;
    }

    Text outputs = TEXT__INIT;
    outputs.n_payload = num_challenges;
    outputs.payload = malloc(sizeof(char*) * outputs.n_payload);
    if (outputs.payload == NULL) {
        log_printf(FATAL, "run_challenges: Failed to malloc");
        exit(1);
    }

    for (int i = 0; i < num_challenges; i++) {
        // Get name of the challenge
        PyObject* pyName = PyList_GetItem(challenge_list, i);
        if (pyName == NULL) {
            PyErr_Print();
            log_printf(ERROR, "run_challenges: cannot get name %d from CHALLENGES list", i);
            outputs.payload[i] = "Fatal error";
            continue;
        }
        const char* func_name = PyUnicode_AsUTF8(pyName);
        if (func_name == NULL) {
            PyErr_Print();
            log_printf(ERROR, "run_challenges: element %d of CHALLENGES is not a Python string", i);
            outputs.payload[i] = "Fatal error";
            continue;
        }
        // Convert input C string to C long then Python tuple args
        long input = strtol(inputs->payload[i], NULL, 10);
        log_printf(DEBUG, "received inputs for %d: %ld", i, input);
        PyObject* arg = Py_BuildValue("(l)", input);
        if (arg == NULL) {
            PyErr_Print();
            log_printf(ERROR, "Couldn't decode input string into Python long for challenge %s", func_name);
            outputs.payload[i] = "Fatal error";
            continue;
        }
        // Run the challenge
        PyObject* ret = NULL;
        if (run_py_function(func_name, NULL, 0, arg, &ret) == 3) {  // Check if challenge got timed out
            for (int j = i; j < num_challenges; j++) {
                // Set rest of challenge outputs to notify the TCP client
                outputs.payload[i] = "Timed out";
            }
            break;
        }
        Py_DECREF(arg);
        // Convert challenge output to Python string then C string
        PyObject* ret_string = PyObject_Str(ret);
        Py_XDECREF(ret);
        if (ret_string == NULL) {
            PyErr_Print();
            log_printf(ERROR, "Couldn't convert return value to Python string for challenge %s", func_name);
            outputs.payload[i] = "Fatal error";
            continue;
        }
        int ret_len;
        const char* c_ret = PyUnicode_AsUTF8AndSize(ret_string, &ret_len);
        Py_DECREF(ret_string);
        outputs.payload[i] = malloc(ret_len + 1);
        if (outputs.payload[i] == NULL) {
            log_printf(FATAL, "run_challenges: Failed to malloc");
            exit(1);
        }
        strcpy(outputs.payload[i], c_ret);  // Need to copy since pointer data is reset each iteration
    }
    text__free_unpacked(inputs, NULL);

    // Send all results back to net handler
    uint16_t len_pb = text__get_packed_size(&outputs);
    uint8_t* send_buf = make_buf(CHALLENGE_DATA_MSG, len_pb);
    uint16_t len_buf = len_pb + BUFFER_OFFSET;
    text__pack(&outputs, send_buf + BUFFER_OFFSET);

    int send_len = sendto(challenge_fd, send_buf, len_buf, 0, (struct sockaddr*) &address, addrlen);
    if (send_len <= 0) {
        log_printf(ERROR, "run_challenges: socket send to challenge_fd failed: %s", strerror(errno));
    } else if (send_len != len_buf) {
        log_printf(WARN, "run_challenges: only %d of %d bytes sent to challenge_fd", send_len, len_buf);
    }

    free(outputs.payload);
    free(send_buf);

    alarm(0);
}


/**
 *  Handler for killing the child mode subprocess
 */
static void python_exit_handler(int signum) {
    // Cancel the Python thread by sending a TimeoutError
    log_printf(DEBUG, "cancelling Python function");
    PyGILState_STATE gstate = PyGILState_Ensure();
    if (mode != CHALLENGE) {
        PyObject* event = PyObject_GetAttrString(pRobot, "sleep_event");
        if (event == NULL) {
            PyErr_Print();
            log_printf(ERROR, "Could not get sleep_event from Robot instance");
            exit(2);
        }
        PyObject* ret = PyObject_CallMethod(event, "set", NULL);
        Py_DECREF(event);
        if (ret == NULL) {
            PyErr_Print();
            log_printf(ERROR, "Could not set sleep_event to True");
            exit(2);
        }
        Py_DECREF(ret);
    }
    PyThreadState_SetAsyncExc((unsigned long) pthread_self(), PyExc_TimeoutError);
    PyGILState_Release(gstate);
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
    int sig_number = WTERMSIG(status);
    log_printf((sig_number == DEBUG_SIGNAL) ? DEBUG : ERROR, "Error when shutting down execution of mode %d", mode);
    log_printf((sig_number == DEBUG_SIGNAL) ? DEBUG : ERROR, "killed by signal %d\n", sig_number);
    // Reset parameters if robot was in AUTO or TELEOP. Needed for safety
    if (mode != CHALLENGE) {
        reset_params();
    }
    mode = IDLE;
}


/**
 *  Creates a new subprocess with fork that will run the given mode using `run_mode` or `run_challenges`.
 */
static pid_t start_mode_subprocess(char* student_code, char* challenge_code) {
    pid_t pid = fork();
    if (pid < 0) {
        log_printf(ERROR, "Failed to create child subprocess for mode %d: %s", mode, strerror(errno));
        return -1;
    } else if (pid == 0) {
        // Now in child process
        signal(SIGINT, SIG_IGN);  // Disable Ctrl+C for child process
        if (mode == CHALLENGE) {
            executor_init(challenge_code);
            signal(SIGTERM, exit);
            signal(SIGALRM, python_exit_handler);  // Sets interrupt for any long-running challenge
            alarm(CHALLENGE_TIME);                 // Set timeout for challenges
            run_challenges();
            robot_desc_write(RUN_MODE, IDLE);  // Will tell parent to call kill_subprocess
        } else {
            executor_init(student_code);
            // signal(SIGTERM, python_exit_handler); // kill subprocess regardless
            run_mode(mode);
        }
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
 *      2: name of the Python file that contains the student challenges functions, without the '.py', Default is "challenges"
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
    char* challenge_code = student_code;  // = "challenges"; change default to "challenges" once Dawn separates challenges into a new Python file
    if (argc > 2) {
        challenge_code = argv[2];
    }

    remove(CHALLENGE_SOCKET);                                  // Always remove any old challenge socket
    struct sockaddr_un my_addr = {AF_UNIX, CHALLENGE_SOCKET};  //for holding IP addresses (IPv4)
    //create the socket
    if ((challenge_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        log_printf(FATAL, "could not create challenge socket: %s", strerror(errno));
        return 1;
    }
    if (bind(challenge_fd, (struct sockaddr*) &my_addr, sizeof(struct sockaddr_un)) < 0) {
        log_printf(FATAL, "challenge socket bind failed: %s", strerror(errno));
        return 1;
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
                pid = start_mode_subprocess(student_code, challenge_code);
                if (pid == -1) {
                    mode = IDLE;
                }
            }
        }
        usleep(100000);  //throttle this thread to ~10 Hz
    }
}
