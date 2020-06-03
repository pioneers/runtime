#ifndef LOGGER_CONFIG_H
#define LOGGER_CONFIG_H

//enumerate logger levels from least to most critical
typedef enum log_level { INFO, DEBUG, WARN, ERROR, FATAL } log_level;

//enumerate names of possible output locations
typedef enum log_output { STD_OUT, LOG_FILE, NETWORK } log_output;

// ************************************ CONFIGURATION *************************** //

//currently printing logs with levels above CURR_LOG_LEVEL (change to allow more or less logs to print)
#define CURR_LOG_LEVEL INFO

//currently printing logs to CURR_OUTPUT_LOC (change to route output to a different location)
#define CURR_OUTPUT_LOC STD_OUT

//define the name/location of the log file
#define LOG_FILE_LOC "/Users/ben/Desktop/runtime_log.log"

#endif