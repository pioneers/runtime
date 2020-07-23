#ifndef EXEC_CLIENT_H
#define EXEC_CLIENT_H

#include "../../runtime_util/runtime_util.h"
#include <signal.h>
#include <sys/wait.h>

void start_executor (char *student_code);

void stop_executor ();

#endif