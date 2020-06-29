#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>  // To get variable arguments
#include <time.h>
#include <errno.h>
#include <fcntl.h>      //for file access permission flags
#include <unistd.h>     //for write to FIFO
#include <sys/stat.h>   //for mkfifo
#include <signal.h>
#include "logger_config.h"
#include "../runtime_util/runtime_util.h"   //(TODO: consider removing relative pathname in include)

// ************************************ PUBLIC LOGGER FUNCTIONS ****************************************** //

//Call function at process start with one of the named processes
void logger_init (process_t process);

//Call before process terminates to clean up logger before exiting
void logger_stop ();

//Call to log something. The message should be a string (string literals work too)
//Level is one of the levels listed in the config file
void log_runtime (log_level level, char *msg);

void log_printf(log_level level, char* format, ...);

#endif