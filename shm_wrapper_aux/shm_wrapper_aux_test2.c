#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include "shm_wrapper_aux.h"
#include "../logger/logger.h"
#include "../runtime_util/runtime_util.h"     //(TODO: consider removing relative pathnames)

//test process 2 for shm_wrapper_aux. is a dummy executor

//sanity gamepad test

//sanity robot description test

//robot description write override test

// *************************************************************************************************** //

void ctrl_c_handler (int sig_num)
{
	printf("Aborting and cleaning up\n");
	fflush(stdout);
	shm_aux_stop(EXECUTOR);
	logger_stop(EXECUTOR);
	exit(1);
}

int main()
{	
	shm_aux_init(EXECUTOR);
	logger_init(EXECUTOR);
	signal(SIGINT, ctrl_c_handler); //hopefully fails gracefully when pressing Ctrl-C in the terminal
	
	sanity_test();	
	
	shm_aux_stop(EXECUTOR);
	logger_stop(EXECUTOR);
	
	sleep(2);
	
	return 0;
}