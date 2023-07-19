#ifndef EXEC_CLIENT_H
#define EXEC_CLIENT_H

#include <errno.h>                // for errno
#include <signal.h>               // for SIGINT
#include <stdio.h>                // for printf, stderr
#include <stdlib.h>               // for malloc, free, realloc, exit
#include <string.h>               // for strcat, strcpy
#include <sys/wait.h>             // for waitpid, pid_t
#include <unistd.h>               // for execlp, fork, chdir
#include "../../logger/logger.h"  // for log_printf

/**
 * Spawns the executor running student_code
 * Arguments:
 *    student_code: name of the student code that you are running
 *        Ex. for a file called example.py, the argument would be "example"
 * No return value.
 */
void start_executor(char* student_code);

/**
 * Stops the executor spawned by start_executor
 */
void stop_executor();

#endif
