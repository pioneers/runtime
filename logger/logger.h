#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include "logger_config.h"

// ************************************ PUBLIC LOGGER FUNCTIONS ****************************************** //

//Call function at process start with one of the named processes
void logger_init (uint8_t process);

//Call before process terminates to clean up logger before exiting
void logger_stop ();

//Call to log something. The message should be a string (string literals work too)
//Level is one of the levels listed in the config file
void log_runtime (uint8_t level, char *msg);

#endif