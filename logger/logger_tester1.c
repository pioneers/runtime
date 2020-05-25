#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "logger.h"

void sanity_test ()
{
	log_runtime(INFO, "here is an info message");
	sleep(1);
	
	log_runtime(DEBUG, "here is a debug message");
	sleep(1);
	
	log_runtime(WARN, "here is a warning message");
	sleep(1);
	
	log_runtime(ERROR, "here is an error message");
	sleep(1);
	
	log_runtime(FATAL, "here is a fatal message");
	sleep(1);
	
	log_runtime(DEBUG, "here is a message with a newline at the end\n");
	log_runtime(WARN, "here is another one in quick succession\n");
}

void simulatenous_prints_test ()
{
	log_runtime(INFO, "here is an info message tester1");
	sleep(1);
	
	log_runtime(DEBUG, "here is a debug message tester1");
	sleep(1);
	
	log_runtime(WARN, "here is a warning message tester1");
	sleep(1);
	
	log_runtime(ERROR, "here is an error message tester1");
	sleep(1);
	
	log_runtime(FATAL, "here is a fatal message tester1");
	sleep(1);
	
	log_runtime(DEBUG, "here is a message with a newline at the end tester1\n");
	log_runtime(WARN, "here is another one in quick succession tester1\n");
}

// *************************************************************************************************** //
void ctrl_c_handler (int sig_num)
{
	printf("Aborting and cleaning up\n");
	fflush(stdout);
	logger_stop();
	exit(1);
}

int main()
{
	logger_init(DEV_HANDLER);
	signal(SIGINT, ctrl_c_handler);
	
	sanity_test();
	
	simulatenous_prints_test();
	
	logger_stop();
	
	return 0;
}