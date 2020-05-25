#ifndef LOGGER_CONFIG_H
#define LOGGER_CONFIG_H

//enumerate logger levels from least to most critical
enum log_levels { INFO, DEBUG, WARN, ERROR, FATAL };

//enumerate names of possible using processes
//TODO: combine with the shared memory wrapper definitions of this!!
enum processes { DEV_HANDLER, EXECUTOR, NET_HANDLER };

//enumerate names of possible output locations
enum outputs { STD_OUT, LOG_FILE, NETWORK };

// ************************************ CONFIGURATION *************************** //

//currently printing logs with levels above CURR_LOG_LEVEL (change to allow more or less logs to print)
#define CURR_LOG_LEVEL INFO

//currently printing logs to CURR_OUTPUT_LOC (change to route output to a different location)
#define CURR_OUTPUT_LOC LOG_FILE

//define the name/location of the log file
#define LOG_FILE_LOC "/Users/ben/Desktop/runtime_log.log"

#endif