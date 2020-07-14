#include "logger.h"

//bit flags for setting logger output location
#define LOG_STDOUT (1 << 0)
#define LOG_FILE (1 << 1)
#define LOG_NETWORK (1 << 2)

#define NUM_CONFIGS 7             //number of configuration parameters in the config file
#define MAX_CONFIG_LINE_LEN 512   //maximum length of a configuration file line, in chars

#define PROCESS_STR_SIZE 32       //size in bytes of the process string

// *********************************** LOGER-SPECIFIC GLOBAL VARS **************************************** //

//general variables
uint8_t OUTPUTS = 0;                       //bitmask that stores where the log outputs should go
char process_str[PROCESS_STR_SIZE];        //string for holding the process name, used in printing
char *log_level_strs[] = {                 //strings for holding names of log levels, used in printing
	"DEBUG",
	"INFO",
	"WARN",
	"PYTHON",
	"ERROR",
	"FATAL"
};
//for holding the requested log levels for the three output locations (all default to DEBUG)
log_level_t stdout_level = DEBUG, file_level = DEBUG, network_level = DEBUG;         

//used for LOG_FILE only
FILE *log_fd = NULL;                        //file descriptor of the log file
char log_file_path[MAX_CONFIG_LINE_LEN];    //file path to the log file

//used for LOG_NETWORK only
mode_t fifo_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;   // -rw-rw-rw permission for FIFO
int fifo_up = 0;      //flag that represents if FIFO is up and running
int fifo_fd = -1;     //file descriptor for FIFO

// ************************************ HELPER FUNCTIONS ****************************************** //

static void set_log_level (log_level_t *level, char *important)
{
	important[strlen(important)] = '\0'; //overwrite the newline at the end
	
	//set level to the contents of important
	if (strcmp(important, "DEBUG") == 0) { *level = DEBUG; } 
	else if (strcmp(important, "INFO") == 0) { *level = INFO; }
	else if (strcmp(important, "WARN") == 0) { *level = WARN; }
	else if (strcmp(important, "ERROR") == 0) { *level = ERROR; }
	else if (strcmp(important, "FATAL") == 0) { *level = FATAL; }
}

static void read_config_file ()
{
	char nextline[MAX_CONFIG_LINE_LEN];
	char not_important[128], important[128];  //for holding information read from the file
	char important_char;
	FILE *conf_fd;
	
	if ((conf_fd = fopen(CONFIG_FILE, "r")) == NULL) {  //open the config file for reading
		perror("fopen: logger could not open config file; exiting...");
		exit(1);
	}
	
	for (int i = 0; i < NUM_CONFIGS; i++) {
		//read until the next line read is not a comment or a blank line
		while (1)  {
			if (fgets(nextline, MAX_CONFIG_LINE_LEN, conf_fd) == NULL) {
				if (feof(conf_fd)) {
					fprintf(stderr, "fgets: end of config file reached before all logger configurations read; exiting...");
					exit(1);
				}
				perror("fgets: error when reading logger config file; exiting...");
				exit(1);
			}
			//check if blank line
			if (nextline[0] == '\n') {
				continue;
			}
			//check if comment line
			if (nextline[0] == '/' && nextline[1] == '/') {
				continue;
			}
			break;
		}
		
		//get the configuration parameter in nextline sequentially
		switch (i) {
			case 0:
				sscanf(nextline, "LOG_TO_STDOUT: %c%s", &important_char, not_important);
				if (important_char == 'Y' || important_char == 'y') {
					OUTPUTS |= LOG_STDOUT;
				}
				break;
			case 1:
				sscanf(nextline, "LOG_TO_FILE: %c%s", &important_char, not_important);
				if (important_char == 'Y' || important_char == 'y') {
					OUTPUTS |= LOG_FILE;
				}
				break;
			case 2:
				sscanf(nextline, "LOG_TO_NETWORK: %c%s", &important_char, not_important);
				if (important_char == 'Y' || important_char == 'y') {
					OUTPUTS |= LOG_NETWORK;
				}
				break;
			case 3:
				sscanf(nextline, "STDOUT_LOG_LEVEL: %s", important);
				set_log_level(&stdout_level, important);
				break;
			case 4:
				sscanf(nextline, "FILE_LOG_LEVEL: %s", important);
				set_log_level(&file_level, important);
				break;
			case 5:
				sscanf(nextline, "NETWORK_LOG_LEVEL: %s", important);
				set_log_level(&network_level, important);
				break;
			case 6:
				sscanf(nextline, "LOG_FILE_LOC: %s", important);
				strcpy(log_file_path, important);
				log_file_path[strlen(log_file_path)] = '\0'; //overwrite the newline at the end
				break;
			default:
				fprintf(stderr, "unknown configuration line: %s\n", nextline);
		}
	}
	
	//close the file descriptor for config file
	fclose(conf_fd);
}

static void sigpipe_handler (int signum)
{
	fifo_up = 0;
}

// ************************************ PUBLIC LOGGER FUNCTIONS ****************************************** //

/*
 * Call function at process start with one of the named processes
 */
void logger_init (process_t process)
{
	int temp_fd;  //temporary file descriptor for opening files if not exist
	
	//read in desired logger configurations
	read_config_file();
	
	//if we want to log to log file, open it for appending
	if (OUTPUTS & LOG_FILE) {
		//call open first with O_CREAT to make sure the log file is created if it doesn't exist
		temp_fd = open(log_file_path, O_RDWR | O_CREAT, 0666);
		close(temp_fd);
		
		//open with fopen to use stdio functions instead
		if ((log_fd = fopen(log_file_path, "a")) == NULL) {  //open the config file for reading
			perror("fopen: logger could not open log file; exiting...");
			exit(1);
		}
	}
	
	//if we want to log to network, create the FIFO pipe if it doesn't exist, and open it for writing
	if (OUTPUTS & LOG_NETWORK) {
		if (mkfifo(LOG_FIFO, fifo_mode) == -1) {
			if (errno != EEXIST) { //don't show error if mkfifo failed because it already exists
				perror("mkfifo: logger create FIFO failed");
			}
		}
		if ((fifo_fd = open(LOG_FIFO, O_WRONLY | O_NONBLOCK)) == -1) {
			if (errno != ENXIO) { //don't show error if open failed because net_handler has not opened pipe for reading yet
				perror("open: logger open FIFO failed");
			}
		} else {
			fifo_up = 1;
		}
		signal(SIGPIPE, sigpipe_handler);
	}
	
	//set the correct process_str for given process
	if (process == DEV_HANDLER) { sprintf(process_str, "DEV_HANDLER"); }
	else if (process == EXECUTOR) { sprintf(process_str, "EXECUTOR"); }
	else if (process == NET_HANDLER) { sprintf(process_str, "NET_HANDLER"); }
	else if (process == SUPERVISOR) { sprintf(process_str, "SUPERVISOR"); }
	else if (process == TEST) { sprintf(process_str, "TEST"); }
}

/* 
 * Call before process terminates to clean up logger before exiting
 */
void logger_stop ()
{	
	//if outputting to stdout, write a newline to stdout
	if (OUTPUTS & LOG_STDOUT) {
		printf("\n");
	}
	
	//if outputting to file, write a new line to log file, then close file descriptor
	if (OUTPUTS & LOG_FILE) {
		fprintf(log_fd, "\n");
		if (fclose(log_fd) != 0) {
			perror("fclose: log file close failed");
		}
	}
	
	//if outputting to network, write a newline to pipe, then close file descriptor
	if ((OUTPUTS & LOG_NETWORK) && (fifo_up == 1)) {
		if (write(fifo_fd, "\n", 1) == -1) {
			//net_handler crashed or was terminated (sent SIGPIPE which was handled)
			if (errno != EPIPE) {
				perror("write: writing to FIFO failed unexpectedly");
			}
		}
		if (close(fifo_fd) != 0) {
			perror("logger close FIFO failed");
		}
	}
}

/* 
 * Logs a message at a specified level to the locations specified in config file
 * Handles format strings (can handle expressions like those in 'printf()')
 * Arguments:
 *    - log_level_t level: one of the levels listed in the enum in this file
 *    - char *format: format string representing message to be formatted
 *    - ...: additional arguments to be formatted in format string
 */
void log_printf (log_level_t level, char *format, ...)
{	
	static time_t now;                   //for holding system time
	static char *time_str;               //for string representation of system time
	static char final_msg[MAX_LOG_LEN];  //final message to be logged to requested locations
	static int len;                      //holding lengths of various strings
	static char msg[MAX_LOG_LEN - 100];  //holds the expanded format string (100 to make room for log header)
	va_list args;                        //this holds the variable-length argument list

	//don't do anything with this message if less than all set levels
	if (level < network_level && level < file_level && level < stdout_level) {
		return;
	}
	
	//expands the format string into msg
	va_start(args, format);
	vsprintf(msg, format, args);
	va_end(args);
	
	//build the message and put into final_msg
	if (level == PYTHON) {
		//print the raw message for PYTHON level
		strcpy(final_msg, msg);
	} else {
		//construct the log message header for all other levels
		now = time(NULL);
		time_str = ctime(&now);
		len = strlen(time_str);
		if (*(time_str + len - 1) == '\n') {
			*(time_str + len - 1) = '\0';
		}
		len = strlen(msg);
		
		//this logic ensures that log messages are separated by exactly one newline (as long as user doesn't put >1 newline)
		if (*(msg + len - 1) == '\n') {
			sprintf(final_msg, "%s @ %s\t(%s) %s", log_level_strs[level], process_str, time_str, msg);
		} else {
			sprintf(final_msg, "%s @ %s\t(%s) %s\n", log_level_strs[level], process_str, time_str, msg);
		}
	}
	
	//send final_msg to the desired output locations
	if ((OUTPUTS & LOG_STDOUT) && (level >= stdout_level)) {
		printf("%s", final_msg);
		fflush(stdout);
	}
	if ((OUTPUTS & LOG_FILE) && (level >= file_level)) {
		fprintf(log_fd, "%s", final_msg);
		fflush(log_fd);
	}
	if ((OUTPUTS & LOG_NETWORK) && (level >= network_level)) {
		//if FIFO is not up, try to connect to it
		if (!fifo_up) {
			if (mkfifo(LOG_FIFO, fifo_mode) == -1) {
				if (errno != EEXIST) { //don't show error if mkfifo failed because it already exists
					perror("mkfifo: logger create FIFO failed");
				}
			}
			if ((fifo_fd = open(LOG_FIFO, O_WRONLY | O_NONBLOCK)) == -1) {
				if (errno != ENXIO) { //don't show error if open failed because net_handler has not opened pipe for reading yet
					perror("open: logger open FIFO failed");
				}
			} else {
				fifo_up = 1;
			}
		}
		//if FIFO is up, try to write to it; if SIGPIPE then use handler to set fifo_up to 0
		if (fifo_up) {
			if (write(fifo_fd, final_msg, strlen(final_msg)) == -1) {
				//net_handler crashed or was terminated (sent SIGPIPE which was handled)
				if (errno != EPIPE) {
					perror("write: writing to FIFO failed unexpectedly");
				}
			}
		}
	}
}
