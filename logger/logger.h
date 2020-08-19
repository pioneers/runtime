  
#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>    // to deal with variable-length argument lists arguments

#include "../runtime_util/runtime_util.h"

#define CONFIG_FILE "logger.config"   // path to logger config file
#define LOG_FIFO "/tmp/log-fifo"      // location of the log FIFO pipe in filesystem

// enumerate logger levels from least to most critical
typedef enum log_level { DEBUG, INFO, WARN, PYTHON, ERROR, FATAL } log_level_t;

// ************************************ PUBLIC LOGGER FUNCTIONS ****************************************** //

/**
 * Call function at process start with one of the named processes
 * Arguments:
 *    process: one of the processes defined in runtime_util; it is used to generate headers that indicate
 *        which process generated any particular log
 */
void logger_init(process_t process);

/**
 * Logs a message at a specified level to the locations specified in config file
 * Handles format strings (can handle expressions like those in 'printf()')
 * Arguments:
 *    level: one of the levels listed in the enum in this file
 *    format: format string representing message to be formatted
 *    ...: additional arguments to be formatted in format string
 */
void log_printf(log_level_t level, char* format, ...);

#endif