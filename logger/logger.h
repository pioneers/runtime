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
#include "../runtime_util/runtime_util.h"   //(TODO: consider removing relative pathname in include)

#define CONFIG_FILE "/home/pi/c-runtime/logger/logger.config"   //path to logger config file

#define LOG_FIFO "/tmp/log-fifo"   //location of the log FIFO pipe in filesystem

//enumerate logger levels from least to most critical
typedef enum log_level { DEBUG, INFO, WARN, PYTHON, ERROR, FATAL } log_level_t;

// ************************************ PUBLIC LOGGER FUNCTIONS ****************************************** //

/*
 * Call function at process start with one of the named processes
 */
void logger_init (process_t process);

/* 
 * Call before process terminates to clean up logger before exiting
 */
void logger_stop ();

/* 
 * Logs a message at a specified level to the locations specified in config file
 * Handles format strings (can handle expressions like those in 'printf()')
 * Arguments:
 *    - log_level_t level: one of the levels listed in the enum in this file
 *    - char *format: format string representing message to be formatted
 *    - ...: additional arguments to be formatted in format string
 */
void log_printf(log_level_t level, char* format, ...);

#endif
