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

//used for NETWORK only
mode_t fifo_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; // -rw-rw-rw permission for FIFO
int fifo_up = 0;  //flag that represents if FIFO is up and running
int fifo_fd = -1; //file descriptor for FIFO

// ************************************ PUBLIC LOGGER FUNCTIONS ****************************************** //

void logger_init (process_t process)
{
	int ret;
	
	//get the appropriate file descriptor
	if (CURR_OUTPUT_LOC == STD_OUT) {
		fd = stdout;
	} else if (CURR_OUTPUT_LOC == LOG_FILE || CURR_OUTPUT_LOC == NETWORK) {
		if ((fd = fopen(LOG_FILE_LOC, "a")) == NULL) {
			perror("log file open failed");
			return;	
		}
	}
	
	//open a FIFO for nonblocking writes
	if (CURR_OUTPUT_LOC == NETWORK) {
		if ((ret = mkfifo(FIFO_NAME, fifo_mode) == -1) && ret != EEXIST) {
			perror("logger create FIFO failed");
		}
		if ((fifo_fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK)) == -1) {
			perror("logger open FIFO failed");
		} else {
			fifo_up = 1;
		}
	}
	
	if (process == DEV_HANDLER) {
		sprintf(process_str, "DEV_HANDLER");
	} else if (process == EXECUTOR) {
		sprintf(process_str, "EXECUTOR");
	} else if (process == NET_HANDLER) {
		sprintf(process_str, "NET_HANDLER");
	} else if (process == SUPERVISOR) {
		sprintf(process_str, "SUPERVISOR");
	} else {
		sprintf(process_str, "TEST");
	}
}

void logger_stop ()
{	
	fprintf(fd, "\n");
	//close the file at fd
	if (CURR_OUTPUT_LOC == STD_OUT) { //don't do anything if logging to stdout
		return;
	} else if (CURR_OUTPUT_LOC == LOG_FILE || CURR_OUTPUT_LOC == NETWORK) {
		if (fclose(fd) != 0) {
			perror("log file close failed");
			return;
		}
	} 
	if (CURR_OUTPUT_LOC == NETWORK && fifo_up == 1) {
		if (close(fifo_fd) != 0) {
			perror("logger close FIFO failed");
		}
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
	static char final_msg[MAX_LOG_LEN];    //only used for sending onto FIFO
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
	
	if (CURR_OUTPUT_LOC == NETWORK && fifo_up == 0) {
		//try to connect to the FIFO
		if ((fifo_fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK)) == -1) {
			;
		} else {
			fifo_up = 1;
		}
	}
	if (CURR_OUTPUT_LOC == NETWORK && fifo_up == 1) {
		if (*(msg + len - 1) == '\n') {
			sprintf(final_msg, "%s @ %s\t(%s) %s", log_level_strs[level], process_str, time_str, msg);
		} else {
			sprintf(final_msg, "%s @ %s\t(%s) %s\n", log_level_strs[level], process_str, time_str, msg);
		}
		if (write(fifo_fd, final_msg, strlen(final_msg)) == -1) {
			//net_handler crashed or was terminated
			if (errno == SIGPIPE) {
				fifo_up = 0;
			} else {
				perror("write: failed to put next log message into FIFO");
			}
		} else {
			return; //if succesful, we're done
		}
	}
	
	//if CURR_OUPUT_LOC == NETWORK and cannot write to pipe, defaults to write to log file
	if (*(msg + len - 1) == '\n') {
		fprintf(fd, "%s @ %s\t(%s) %s", log_level_strs[level], process_str, time_str, msg);
	} else {
		fprintf(fd, "%s @ %s\t(%s) %s\n", log_level_strs[level], process_str, time_str, msg);
	}
	fflush(fd);
}


/**
 *	Provides same printing functionality as `printf` but prints instead to the log with the specified log level.
 */
void log_printf (log_level level, char *format, ...)
{
	char msg[MAX_LOG_LEN];
	va_list args; //this holds the variable-length argument list
	
    va_start(args, format);
    vsprintf(msg, format, args); //formats the input message
	log_runtime(level, msg);     //use log_runtime to write formatted string to log file
    va_end(args);
}


