#ifndef LOGGER_CONFIG_H
#define LOGGER_CONFIG_H

//enumerate logger levels from least to most critical
typedef enum log_level { DEBUG, INFO, WARN, ERROR, FATAL, PYTHON } log_level;

//enumerate names of possible output locations
typedef enum log_output { STD_OUT, LOG_FILE, NETWORK } log_output;

// ************************************ CONFIGURATION *************************** //

//currently printing logs with levels above CURR_LOG_LEVEL (change to allow more or less logs to print)
#define CURR_LOG_LEVEL DEBUG

//currently printing logs to CURR_OUTPUT_LOC (change to route output to a different location)
#define CURR_OUTPUT_LOC NETWORK

//define the name/location of the log file (for NETWORK and LOG_FILE)
#define LOG_FILE_LOC "/home/pi/c-runtime/logger/test.log"

//define the name of the FIFO pipe (for NETWORK only)
#define FIFO_NAME "/tmp/runtime-fifo"

#endif
