#include "executor.h"

// Global variables to all threads
char* loader_file = "code_loader.py";
char* module_name;
PyObject* pModule;
pthread_mutex_t module_mutex = PTHREAD_MUTEX_INITIALIZER; // Not sure if needed


typedef struct file_args {
    char* file_name;
    char* mode;
} file_args_t;

typedef struct thread_args {
    char* func_name;
} thread_args_t;

void sigintHandler(int sig_num) {
	printf("===Terminating using Ctrl+C===\n");
    // fflush(stdout);
    executor_stop();
	exit(0);
}

void sigalrmHandler(int sig_num) {
    log_runtime(DEBUG, "in alarm handler");
    // pthread_exit(NULL);
    // pthread_cancel(pthread_self());
    pthread_kill(pthread_self(), SIGTERM);
    log_runtime(DEBUG, "just exited");
}

void executor_init(char* student_code) {
    logger_init(EXECUTOR);
    // shm_init(EXECUTOR);
    module_name = student_code;
    Py_Initialize();
}

void executor_stop() {
    Py_XDECREF(pModule);
    Py_FinalizeEx();
    // shm_stop(EXECUTOR);
    logger_stop();
}


void call_thread_timeout(char* func_name, int timeout) {
    pthread_t tid;
}


void run_py_function(void* func_name) {
    pthread_mutex_lock(&module_mutex);
    if (pModule == NULL) {
        pModule = PyImport_ImportModule(module_name);
    }
    if (pModule == NULL) {
        PyErr_Print();
        char msg[64];
        sprintf(msg, "Could not import module: %s", module_name);
        log_runtime(ERROR, msg);
        return;
    }

    char* func = (char*) func_name;
    PyObject* pFunc = PyObject_GetAttrString(pModule, func);
    pthread_mutex_unlock(&module_mutex);
    /* pFunc is a new reference */

    printf("Is callable: %d\n", PyCallable_Check(pFunc));

    if (pFunc && PyCallable_Check(pFunc)) {
        PyObject* pArgs = PyTuple_New(2);

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
}


void* run_student_file(void* args) {
    /* allow the thread to be killed at any time */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    // file_args_t* args = (file_args_t*) thread_args;
    char* mode = (char*) args;
    FILE* file = fopen(loader_file, "r");
    wchar_t* py_args[3] = {
        Py_DecodeLocale("code_loader", NULL),
        Py_DecodeLocale(module_name, NULL),
        Py_DecodeLocale(mode, NULL)
    };
    PySys_SetArgvEx(3, py_args, 0);
    PyRun_SimpleFile(file, loader_file);
}


void start_mode(mode mode) {
    if (mode == AUTO) {

    }  
    else if (mode == TELEOP) {

    }
    else if (mode == ESTOP) {
        executor_stop();
    }
}

void run_file_subprocess(mode mode) {

}

void start_loader(mode mode) {
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
    // pthread_join(tid, NULL);
}


int main(int argc, char* argv[]) {
    signal(SIGINT, sigintHandler);
    signal(SIGALRM, sigalrmHandler);
    executor_init("studentcode");
    // run_student_file("autonomous");
    start_loader(AUTO);
    pthread_sigmask()
    // run_py_function("test_api");
    while(1) {

    }
    return 0;
}