#define PY_SSIZE_T_CLEAN
#include <Python.h>                        // For Python's C API
#include <stdio.h>                         //for i/o
#include <stdlib.h>                        //for standard utility functions (exit, sleep)
#include <sys/types.h>                     //for sem_t and other standard system types
#include <sys/syscall.h>
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
const char *api_module = "studentapi";
char *student_module;
PyObject *pModule, *pAPI, *pPrint, *pRobot, *pGamepad;
pthread_t mode_thread, handler_thread;

// Timings for all modes
struct timespec setup_time = { 2, 0 };    // Max time allowed for setup functions
#define freq 10.0 // Minimum number of times per second the main loop should run
struct timespec main_interval = { 0, (long) ((1.0 / freq) * 1e9) };

/**
 *  Struct used as input to all threads. Not all threads will use all arguments.
 */
typedef struct thread_args {
    char *func_name;
    pthread_cond_t *cond;
    char *mode;
    struct timespec *timeout;
    int loop;
} thread_args_t;

struct cleanup {
    PyObject *func;
    PyObject *value;
};


/**
 *  Returns the appropriate string representation from the given mode.
 */
const char *get_mode_str(robot_desc_val_t mode) {
    if (mode == AUTO) {
        return "autonomous";
    }
    else if (mode == TELEOP) {
        return "teleop";
    }
    else if (mode == IDLE) {
        return "idle";
    }
    return NULL;
}


/**
 *  Closes all executor processes and exits cleanly.
 */
void executor_stop() {
	printf("\nShutting down executor...\n");
    pthread_cancel(mode_thread);
    pthread_join(mode_thread, NULL);
    log_runtime(DEBUG, "joining to make sure thread is cancelled");

    Py_XDECREF(pAPI);
    Py_XDECREF(pPrint);
    Py_XDECREF(pGamepad);
    Py_XDECREF(pRobot);
    Py_XDECREF(pModule);
    // Py_FinalizeEx();
    shm_aux_stop(EXECUTOR);
    log_runtime(DEBUG, "Aux SHM stopped");
    shm_stop(STUDENTAPI);
    log_runtime(DEBUG, "SHM stopped");
    logger_stop();
	exit(1);
}


/**
 *  Handler for keyboard interrupts SIGINT (Ctrl + C)
 */
void sigintHandler(int signum) {
    executor_stop();
}


/**
 *  Initializes the executor process. Must be the first thing called.
 *
 *  Input: 
 *      student_code: string representing the name of the student's Python file, without the .py
 */
void executor_init(char *student_code) {
    //initialize
	logger_init(EXECUTOR);
    shm_aux_init(EXECUTOR);
    log_runtime(DEBUG, "Aux SHM initialized");
    shm_init(STUDENTAPI);
    log_runtime(DEBUG, "SHM intialized");
    student_module = student_code;
    Py_Initialize();
    PyEval_InitThreads();
    // Need this so that the Python interpreter sees the Python files in this directory
    PyRun_SimpleString("import sys;sys.path.insert(0, '.')");
	
	//imports the Cython student API
    pAPI = PyImport_ImportModule(api_module);
    if (pAPI == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Could not import API module");
        executor_stop();
    }
	
	//checks to make sure there is a print function
    pPrint = PyObject_GetAttrString(pAPI, "_print");
    if (pPrint == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Could not find _print");
        executor_stop();
    }
	
	//checks to make sure there is a Robot class, then instantiates it
    PyObject* robot_class = PyObject_GetAttrString(pAPI, "Robot");
    if (robot_class == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Could not find Robot class");
        executor_stop();
    }
    pRobot = PyObject_CallObject(robot_class, NULL);
    if (pRobot == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Could not instantiate Robot");
        executor_stop();
    }
    Py_DECREF(robot_class);
	
	//checks to make sure there is a Gamepad class, then instantiates it
    PyObject* gamepad_class = PyObject_GetAttrString(pAPI, "Gamepad");
    if (gamepad_class == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Could not find Gamepad class");
        executor_stop();
    }
    const char *mode_str = get_mode_str(robot_desc_read(RUN_MODE));
    pGamepad = PyObject_CallFunction(gamepad_class, "s", mode_str);
    if (pGamepad == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Could not instantiate Gamepad");
        executor_stop();
    }
    Py_DECREF(gamepad_class);
	
	//imports the student code
    pModule = PyImport_ImportModule(student_module);
    if (pModule == NULL) {
        PyErr_Print();
        char msg[64];
        sprintf(msg, "Could not import module: %s", student_module);
        log_runtime(ERROR, msg);
        executor_stop();
    }
    
    int err = PyObject_SetAttrString(pModule, "Robot", pRobot);
    err |= PyObject_SetAttrString(pModule, "Gamepad", pGamepad);
    if (err != 0) {
        PyErr_Print();
        log_runtime(ERROR, "Could not insert API into student code.");
        executor_stop();
    }
}


/**
 *  Thread cleanup function to decref Python objects. 
 * 
 *  Inputs:
 *      args: a PyObject** that needs their reference decremented
 */
void py_function_cleanup(void *args) {
    struct cleanup *obj = (struct cleanup *) args;
    Py_XDECREF(obj->func);
    Py_XDECREF(obj->value);
}


/**
 *  Thread cleanup function to release GIL.
 *  
 *  Inputs:
 *      args: a PyGILState_STATE* to release
 */
void gil_cleanup(void* args) {
    PyGILState_STATE* gil = (PyGILState_STATE*) args;
    PyGILState_Release(*gil);
    log_runtime(DEBUG, "released GIL");
}


/**
 *  Runs the Python function specified in the arguments.
 * 
 *  Behavior: If loop = 0, this will block the calling thread for the length of
 *  the Python function call. If loop is nonzero, this will block the calling thread forever.
 *  This function should be run in a separate thread.
 * 
 *  Inputs: Uses thread_args_t as input struct.
 *      Necessary fields:
 *          func_name: string of function name to run in the student code
 *          mode: string of the current mode
 *          loop: boolean for whether the Python function should be called in a while loop forever
 *      Optional fields:
 *          cond: condition variable that blocks the calling thread, used with `run_function_timeout`
 *          timeout: max length of execution time before a warning is issued for the function call
 */
void* run_py_function(void *thread_args) {
    thread_args_t *args = (thread_args_t *)thread_args;

    PyGILState_STATE gstate = PyGILState_Ensure();
    log_runtime(DEBUG, "Acquired GIL");
    pthread_cleanup_push(gil_cleanup, (void*) &gstate);
	
	//assign the run mode to the Gamepad object
    PyObject *pMode = PyUnicode_FromString(args->mode);
    int err = PyObject_SetAttrString(pGamepad, "mode", pMode);
    Py_DECREF(pMode);
    if (err != 0) {
        PyErr_Print();
        char msg[128];
        sprintf(msg, "Couldn't assign mode for Gamepad while trying to run %s", args->func_name);
        log_runtime(ERROR, msg);
        if (args->cond != NULL) {
            pthread_cond_signal(args->cond);
        }
        return NULL;
    }
	
    //retrieve the Python function from the student code
    PyObject *pFunc = PyObject_GetAttrString(pModule, args->func_name);
    PyObject *pValue = NULL;
    if (pFunc && PyCallable_Check(pFunc)) {
        struct timespec start, end;
        long time, max_time;
        if (args->timeout) {
            max_time = args->timeout->tv_sec * 1e9 + args->timeout->tv_nsec;
        }
        //Necessary cleanup to handle pthread_cancel 
        struct cleanup objects = { pFunc, pValue };
        pthread_cleanup_push(py_function_cleanup, (void *) &objects);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
		
        do {
            clock_gettime(CLOCK_MONOTONIC, &start);
            pValue = PyObject_CallObject(pFunc, NULL); //make call to Python function
            clock_gettime(CLOCK_MONOTONIC, &end);

			//if the time the Python function took was greater than max_time, warn that it's taking too long
            time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
            if (args->loop && args->timeout && time > max_time) {
                char msg[128];
                sprintf(msg, "Function %s is taking longer than %lu milliseconds, indicating a loop in the code.", args->func_name, (long) (max_time / 1e6));
                log_runtime(WARN, msg);
            }
			//catch execution error
            if (pValue == NULL) {
                if (PyErr_Occurred()) {
                    PyErr_Print();
                }
                char msg[128];
                sprintf(msg, "Python function %s call failed", args->func_name);
                log_runtime(ERROR, msg);
                break;
            }
            else {
                Py_DECREF(pValue);
            }
        } while(args->loop);
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
        // Calls Python cleanup handler
        pthread_cleanup_pop(1);
    }
    else {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        char msg[128];
        sprintf(msg, "Cannot find function in student code: %s\n", args->func_name);
        log_runtime(ERROR, msg);
    }
    // Releases the GIl since we're done calling Python
    pthread_cleanup_pop(1);
	
    // Send the signal that the thread is done, using the condition variable.
    if (args->cond != NULL) {
        pthread_cond_signal(args->cond);
    }
    return NULL;
}


/**
 *  Runs the specified Python function in another thread with the specified timeout.
 *  Uses `run_py_function` as the underlying thread. Meant to be used for the setup functions.
 * 
 *  Behavior: This is a blocking function and will block the calling thread for up to the length of the timeout.
 *  This function will be called standalone by `run_mode_functions`.
 * 
 *  Inputs: Uses thread_args_t as input struct. Fields necessary are:
 *      func_name: string of function name to run in the student code
 *      mode: string of the current mode
 *      timeout: timespec of the max timeout the function should run
 */
void run_function_timeout(thread_args_t *args) {
    pthread_t tid;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    args->cond = &cond;
    
	//calculate the timeout time of the function
	struct timespec time;
    pthread_mutex_lock(&mutex);
    clock_gettime(CLOCK_REALTIME, &time);
    time.tv_sec += args->timeout->tv_sec;
    time.tv_nsec += args->timeout->tv_nsec;
	    
	//create the thread and start running the function
    pthread_create(&tid, NULL, run_py_function, (void*) args);
    //set the timeout for the thread and wait
    int err = pthread_cond_timedwait(&cond, &mutex, &time);
    if (err == ETIMEDOUT) {
        char msg[128];
        sprintf(msg, "Function %s is taking longer than %lu seconds, indicating a loop in the code.", args->func_name, args->timeout->tv_sec);
        log_runtime(WARN, msg);
    }
    else if (err != 0) {
        char msg[128];
        sprintf(msg, "Function %s failed with error code %d", args->func_name, err);
        log_runtime(ERROR, msg);
        perror(msg);
    }
	
	//remove everything associated with the thread
    pthread_join(tid, NULL);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}


void action_cleanup(void* args) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject* running_actions = PyObject_GetAttrString(pRobot, "running_actions");
    if (running_actions == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "getting running actions failed");
        return;
    }
    PyObject* actions = PyMapping_Values(running_actions);
    Py_DECREF(running_actions);
    if (actions == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Getting running threads failed");
        return;
    }
    Py_ssize_t num_actions = PyList_Size(actions);
    for (int i = 0; i < num_actions; i++) {
        PyObject* thread = PyList_GetItem(actions, i);
        if (thread == NULL) {
            PyErr_Print();
            log_runtime(ERROR, "Get item from running_actions failed");
            continue;
        }

        PyObject* alive = PyObject_CallMethod(thread, "is_alive", NULL);
        if (alive == NULL) {
            PyErr_Print();
            log_runtime(ERROR, "Couldn't get whether thread is alive");
            continue;
        }
        else if (PyObject_Not(alive)) {
            continue;
        }

        PyObject* pTID = PyObject_GetAttrString(thread, "ident");
        if (pTID == NULL) {
            PyErr_Print();
            log_runtime(ERROR, "Failed to get ident from thread");
            continue;
        }

        long tid = PyLong_AsLongLong(pTID);
        if (tid == -1) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            log_runtime(ERROR, "Failed to convert PyLong to C long");
            continue;
        }

        PyThreadState_SetAsyncExc(tid, PyExc_TimeoutError);
    }
    PyGILState_Release(gstate);
    log_runtime(DEBUG, "Stopped student actions");
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
void* run_mode_functions(void *args) {
	//Set up the arguments to the threads that will run the setup and main threads
    char *mode = (char *)args;
    char setup_str[20], main_str[20];
    sprintf(setup_str, "%s_setup", mode);
    sprintf(main_str, "%s_main", mode);
    thread_args_t setup_args = { setup_str, NULL, mode, &setup_time, 0 };
    thread_args_t main_args = { main_str, NULL, mode, &main_interval, 1 };
	
	//Ensure that SIGINT is only handled by main thread
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
	
    //Adds cleanup function to stop student actions once mode is finished
    pthread_cleanup_push(action_cleanup, NULL);
	//Run the setup function on a timeout, followed by main function on a loop
    run_function_timeout(&setup_args);
    run_py_function(&main_args);
    pthread_cleanup_pop(1);
    return NULL;
}


/**
 *  Main bootloader that calls `run_mode_functions` with the correct mode. Ensures any previously running
 *  thread is terminated first.
 * 
 *  Behavior: This is a blocking function and will begin handling the run mode forever until a SIGINT.
 */
int main(int argc, char *argv[]) {
    signal(SIGINT, sigintHandler);
    executor_init("studentcode"); // Default name of student code file 
    Py_BEGIN_ALLOW_THREADS
    robot_desc_val_t mode = IDLE;
    robot_desc_val_t new_mode = IDLE;

    // Main loop that checks for new run mode in shared memory from the network handler
    do {
        // If we receive a new mode, cancel the previous mode and start the new one
        if (new_mode != mode) {
            mode = new_mode;
            pthread_cancel(mode_thread);
            pthread_join(mode_thread, NULL);
            if (mode != IDLE) {
                pthread_create(&mode_thread, NULL, run_mode_functions, (void*) get_mode_str(mode));
            }
        }
        new_mode = robot_desc_read(RUN_MODE);
    } while (1);

    // Test complete match
    // pthread_create(&mode_thread, NULL, run_mode_functions, (void*) "autonomous");
    // sleep(4);
    // pthread_cancel(mode_thread);
    // pthread_join(mode_thread, NULL);
    // log_runtime(DEBUG, "auton ended");
    // pthread_create(&mode_thread, NULL, run_mode_functions, (void*) "teleop");
    // while(1) {}

    Py_END_ALLOW_THREADS
    return 0;
}
