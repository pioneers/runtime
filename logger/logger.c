#include "logger.h"

#define PROCESS_STR_SIZE 32  //size in bytes of the process string

// *********************************** LOGER-SPECIFIC GLOBAL VARS **************************************** //

FILE *fd = NULL;                           //file descriptor to refer to output location
char process_str[PROCESS_STR_SIZE];        //string for holding the process name, used in printing
char *log_level_strs[] = {                 //strings for holding names of log levels, used in printing
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"FATAL"
};

// ************************************ PUBLIC LOGGER FUNCTIONS ****************************************** //

void logger_init (process_t process)
{
	//get the appropriate file descriptor
	if (CURR_OUTPUT_LOC == STD_OUT) {
		fd = stdout;
	} else if (CURR_OUTPUT_LOC == LOG_FILE) {
		if ((fd = fopen(LOG_FILE_LOC, "a")) == NULL) {
			perror("log file open failed");
			return;	
		}
	} else {
		//TODO: add sending over network functionality
		;
	}
	
	if (process == DEV_HANDLER) {
		sprintf(process_str, "DEV_HANDLER");
	} else if (process == EXECUTOR) {
		sprintf(process_str, "EXECUTOR");
	} else if (process == NET_HANDLER) {
		sprintf(process_str, "NET_HANDLER");
	} else if (process == SUPERVISOR) {
		sprintf(process_str, "SUPERVISOR");
	}
	else {
		sprintf(process_str, "TEST");
	}
}

void logger_stop ()
{	
	fprintf(fd, "\n");
	//close the file at fd
	if (CURR_OUTPUT_LOC == STD_OUT) { //don't do anything if logging to stdout
		return;
	} else if (CURR_OUTPUT_LOC == LOG_FILE) {
		if (fclose(fd) != 0) {
			perror("log file close failed");
			return;
		}
	} else {
		//TODO: add sending over network functionality
	}
}

//if you want to format messages, you need to sprintf them into a string
//and then call log with that string. We are not trying to replicate the
//entire format string syntax of printf / sprintf here
void log_runtime (log_level level, char *msg)
{
	if (fd == NULL) {
		fprintf(stderr, "ERROR: calling logger print without initializing logger \n");
		exit(1);
	}
	static time_t now;
	static char *time_str;
	static int len;
	
	//don't do anything with this message if less than CURR_LOG_LEVEL
	if (level < CURR_LOG_LEVEL) {
		return;
	}
	
	//format the message and send it to the file
	now = time(NULL);
	time_str = ctime(&now);
	len = strlen(time_str);
	if (*(time_str + len - 1) == '\n') {
		*(time_str + len - 1) = '\0';
	}
	
	len = strlen(msg);
	if (level == PYTHON) {
		fprintf(fd, "%s", msg);
	}
	else if (*(msg + len - 1) == '\n') {
		fprintf(fd, "%s @ %s\t(%s) %s", log_level_strs[level], process_str, time_str, msg);
	} else {
		fprintf(fd, "%s @ %s\t(%s) %s\n", log_level_strs[level], process_str, time_str, msg);
	}
	fflush(fd);
	
}


/**
 *	Provides same printing functionality as `printf` but prints instead to the log with the specified log level.
 */
void log_printf(log_level level, char* format, ...) {
	va_list args;
    va_start(args, format);
	char msg[MAX_LOG_LEN];
    vsprintf(msg, format, args);
	log_runtime(level, msg);
    va_end(args);
}


