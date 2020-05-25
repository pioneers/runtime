# Runtime Logger

The Runtime Logger is a simple, lightweight interface for the various processes in Runtime to report problems or status updates and collect them in one location. The output can be routed to standard output, to a file, or (not yet implemented) sent on the network to Dawn/Shepherd.

## Description

`logger.h` contains the header file detailing the available functions for the logger. Please read the comments written here and consult the tester files for an overview of the wrapper's usage. Source code for the logger is found in `logger.c`. 

`logger_config.h` contains several constants that are important for logger operation. The list of log levels are enumerated here (INFO, DEBUG, WARN, ERROR, FATAL), as well as the possible locations to output the logs (STD_OUT, LOG_FILE, NETWORK). Set the lowest desired log level to be printed, output location, and name/location of log file in `logger_config.h`.

`logger_tester1.c` and `logger_tester2.c` contain simple sanity checks to demonstrate that the logger is functioning. First, configure the logger as desired in `logger_config.h`. Then compile with
```bash
gcc logger_tester1.c logger.c -o tester1
gcc logger_tester2.c logger.c -o tester2
```
Then run by opening two separate terminal windows and running `./tester1` in one window and `./tester2` in the other.

## Todos

Sending logs onto the network has not been implemented (likely needs to be written alongside the network handler).

Potentially add custom filters for logs (printing between certain times, printing only from one process, printing at different log levels for different processes, etc.).
