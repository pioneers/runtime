#ifndef EXECUTOR_H
#define EXECUTOR_H

#define PY_SSIZE_T_CLEAN
#include <python3.6/Python.h>              // For Python's C API
#include <stdio.h>                         //for i/o
#include <stdlib.h>                        //for standard utility functions (exit, sleep)
#include <sys/types.h>                     //for sem_t and other standard system types
#include <stdint.h>                        //for standard int types
#include <unistd.h>                        //for sleep
#include <pthread.h>                       //for POSIX threads
#include <signal.h>                        // Used to handle SIGTERM, SIGINT, SIGKILL
#include "../runtime_util/runtime_util.h"  //for runtime constants (TODO: consider removing relative pathname in include)
#include "../shm_wrapper/shm_wrapper.h"    // Shared memory wrapper to get/send device data
#include "../logger/logger.h"              // for runtime logger

#define MAX_THREADS 32

// To handle a keyboard interrupt
void sigintHandler(int sig_num);

// Initialization method for executor
void executor_init();
void executor_stop();
void start_mode(mode mode);


#endif

