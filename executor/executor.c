#include "executor.h"

// Global variables to all threads
const char* loader_file = "code_loader.py";
const char* api_module = "studentapi";
char* student_module;
PyObject *pModule, *pAPI, *pPrint, *pRobot, *pGamepad;
pthread_mutex_t module_mutex = PTHREAD_MUTEX_INITIALIZER; // Not sure if needed

typedef struct thread_args {
    char* func_name;
    pthread_cond_t* cond_p;
    uint8_t done;
    int timeout;
    char* mode;
} thread_args_t;


char* get_mode_str() {
    robot_desc_val_t mode = robot_desc_read(RUN_MODE);
    if (mode == AUTO) {
        return "autonomous";
    }
    else if (mode == TELEOP) {
        return "teleop";
    }
    else if (mode == IDLE) {
        return "idle";
    }
}


void exitHandler() {
	printf("\nShutting down executor...\n");
    fflush(stdout);
    executor_stop();
	exit(1);
}

void sigintHandler(int signum) {
    exitHandler();
}


void executor_init(char* student_code) {
    logger_init(EXECUTOR);
    shm_init(EXECUTOR);
    shm_aux_init(EXECUTOR);
    student_module = student_code;
    Py_Initialize();
    PyRun_SimpleString("import sys;sys.path.insert(0, '.')");

    pAPI = PyImport_ImportModule(api_module);
    if (pAPI == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Could not import API module");
        exitHandler();
    }

    pPrint = PyObject_GetAttrString(pAPI, "_print");
    if (pPrint == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Could not find _print");
        exitHandler();
    }

    PyObject* robot_class = PyObject_GetAttrString(pAPI, "Robot");
    if (robot_class == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Could not find Robot class");
        exitHandler();
    }
    pRobot = PyObject_CallObject(robot_class, NULL);
    if (pRobot == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Could not instantiate Robot");
        exitHandler();
    }
    Py_DECREF(robot_class);

    PyObject* gamepad_class = PyObject_GetAttrString(pAPI, "Gamepad");
    if (gamepad_class == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Could not find Gamepad class");
        exitHandler();
    }
    pGamepad = PyObject_CallFunction(gamepad_class, "s", get_mode_str());
    if (pGamepad == NULL) {
        PyErr_Print();
        log_runtime(ERROR, "Could not instantiate Gamepad");
        exitHandler();
    }
    Py_DECREF(gamepad_class);
    pModule = PyImport_ImportModule(student_module);
    if (pModule == NULL) {
        PyErr_Print();
        char msg[64];
        sprintf(msg, "Could not import module: %s", student_module);
        log_runtime(ERROR, msg);
        exitHandler();
    }
    
    int flag = PyObject_SetAttrString(pModule, "print", pPrint);
    flag |= PyObject_SetAttrString(pModule, "Robot", pRobot);
    flag |= PyObject_SetAttrString(pModule, "Gamepad", pGamepad);
    if (flag != 0) {
        PyErr_Print();
        log_runtime(ERROR, "Could not insert API into student code.");
        exitHandler();
    }
}


void executor_stop() {
    Py_XDECREF(pModule);
    Py_XDECREF(pAPI);
    Py_XDECREF(pPrint);
    Py_XDECREF(pRobot);
    Py_XDECREF(pGamepad);
    Py_FinalizeEx();
    shm_aux_stop(EXECUTOR);
    shm_stop(EXECUTOR);
    logger_stop();
}


void* run_py_function(void* thread_args) {
    /* allow the thread to be killed at any time */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    thread_args_t* args = (thread_args_t*) thread_args;
    char* func = args->func_name;
    pthread_cond_t* cond = args->cond_p;
    char* mode = args->mode;

    // Ensure only 1 thread can access the module at a time. Not really sure if needed
    pthread_mutex_lock(&module_mutex);
    PyObject* pMode = PyUnicode_FromString(mode);
    int err = PyObject_SetAttrString(pGamepad, "mode", pMode);
    if (err != 0) {
        log_runtime(ERROR, "Couldn't assign run mode");
        return NULL;
    }
    Py_DECREF(pMode);

    PyObject* pFunc = PyObject_GetAttrString(pModule, func);
    pthread_mutex_unlock(&module_mutex);

    if (pFunc && PyCallable_Check(pFunc)) {
        // PyObject* pArgs = PyTuple_New(2);
        PyObject* pValue = PyObject_CallObject(pFunc, NULL);
        Py_DECREF(pFunc);
        if (pValue == NULL) {
            PyErr_Print();
            char msg[64];
            sprintf(msg, "Function %s call failed", func);
            log_runtime(ERROR, msg);
        }
        else {
            Py_DECREF(pValue);
        }
    }
    else {
        if (PyErr_Occurred())
            PyErr_Print();
        char msg[64];
        sprintf(msg, "Cannot find function: %s\n", func);
        log_runtime(ERROR, msg);
    }
    args->done = 1;
    // Send the signal that the thread is done using the condition variable.
    pthread_cond_signal(cond);
    return NULL;
}


void* call_function_timeout(void* thread_args) {
    thread_args_t* args = (thread_args_t*) thread_args;
    pthread_t tid;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    args->cond_p = &cond;
    struct timespec time;
    pthread_mutex_lock(&mutex);
    clock_gettime(CLOCK_MONOTONIC, &time);
    time.tv_sec += args->timeout;
    pthread_create(&tid, NULL, run_py_function, (void*) args);
    int err;
    while (args->done == 0) {
        err = pthread_cond_timedwait(&cond, &mutex, &time);
    }
    if (err != 0 && err != ETIMEDOUT) {
        log_runtime(ERROR, "Function call failed.");
    }
    pthread_mutex_unlock(&mutex);
    pthread_cancel(tid);
    // pthread_exit(NULL);
}


void* run_student_file(void* args) {
    /* allow the thread to be killed at any time */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    // file_args_t* args = (file_args_t*) thread_args;
    FILE* file = fopen(loader_file, "r");
    wchar_t* py_args[3] = {
        Py_DecodeLocale("code_loader", NULL),
        Py_DecodeLocale(student_module, NULL),
        Py_DecodeLocale(get_mode_str(), NULL)
    };
    PySys_SetArgvEx(3, py_args, 0);
    PyRun_SimpleFile(file, loader_file);
}


void* run_file_subprocess(void* args) {
    char command[256];
    char* mode = (char*) args;
    sprintf(command, "python3.6 %s %s %s", loader_file, student_module, mode);
    FILE* process = popen(command, "r");
    if (process == NULL) {
        log_runtime(ERROR, "Failed to start process for");
        log_runtime(ERROR, command);
    }
    // For debugging only
    while (fgets(command, 256, process) != NULL) {
        printf("%s", command);
    }
    int status;
    if ((status = pclose(process)) != 0) {
        log_runtime(ERROR, "Error occurred while calling process");
        log_runtime(ERROR, command);
    }
}


void start_loader_subprocess(robot_desc_val_t mode) {
    pthread_t tid;
    char* mode_str;
    if (mode == AUTO) {
        mode_str = "autonomous";
    }
    else if (mode == TELEOP) {
        mode_str = "teleop";
    }
    log_runtime(DEBUG, "Now creating subprocess");
    pthread_create(&tid, NULL, run_file_subprocess, (void*) mode_str);
}


void start_loader_thread(robot_desc_val_t mode) {
    pthread_t tid;
    char* mode_str;
    if (mode == AUTO) {
        mode_str = "autonomous";
    }
    else if (mode == TELEOP) {
        mode_str = "teleop";
    }
    log_runtime(DEBUG, "Now creating thread");
    pthread_create(&tid, NULL, run_student_file, (void*) mode_str);
}


void start_mode(robot_desc_val_t mode) {
    if (mode == AUTO) {

    }  
    else if (mode == TELEOP) {

    }
    else if (mode == ESTOP) {
        executor_stop();
    }
}


int main(int argc, char* argv[]) {
    signal(SIGINT, sigintHandler);
    // signal(SIGALRM, sigalrmHandler);
    executor_init("studentcode");
    struct timespec start, end;

    // clock_gettime(CLOCK_MONOTONIC, &start);
    // run_student_file("autonomous");
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // printf("Elapsed milliseconds for file: %lu \n", (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000);

    // clock_gettime(CLOCK_MONOTONIC, &start);
    // run_file_subprocess(AUTO);
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // printf("Elapsed milliseconds for subprocess: %lu \n", (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000);    

    // start_loader_thread(AUTO);

    // start_loader_subprocess(AUTO);
    // log_runtime(DEBUG, "Subprocess has started");

    thread_args_t args = {"autonomous_setup", NULL, 0, 1, "AUTO"};
    // call_function_timeout((void*) &args);
    // call_function_timeout("autonomous_main", 4);

    pthread_t id;
    pthread_create(&id, NULL, call_function_timeout, (void*) &args);
    sleep(2);
    args.func_name = "autonomous_main";
    args.done = 0;
    args.timeout = 5;
    pthread_create(&id, NULL, call_function_timeout, (void*) &args);
    

    // sleep(1);
    // student_module = "test_studentapi";
    // run_py_function("test_api");


    while(1) {

    }
    return 0;
}