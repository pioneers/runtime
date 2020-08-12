#ifndef EXEC_CLIENT_H
#define EXEC_CLIENT_H

#include "../../runtime_util/runtime_util.h"
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

/*
 * Spawns the executor running student_code
 * Arguments:
 *    - char *student_code: name of the student code that you are running
 *        Ex. for a file called example.py, the argument would be "example"
 * No return value.
 */
void start_executor (char *student_code, char *challenge_code);

/*
 * Stops the executor spawned by start_executor
 */
void stop_executor ();

#endif
