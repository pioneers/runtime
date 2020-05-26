#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include "logger_config.h"
#include "../runtime_util.h"   //(TODO: consider removing relative pathname in include)

// ************************************ PUBLIC LOGGER FUNCTIONS ****************************************** //

//Call function at process start with one of the named processes
void logger_init (process_t process);

//Call before process terminates to clean up logger before exiting
void logger_stop ();

//Call to log something. The message should be a string (string literals work too)
//Level is one of the levels listed in the config file
void log_runtime (log_level level, char *msg);

#endif