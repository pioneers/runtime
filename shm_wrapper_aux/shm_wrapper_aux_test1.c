#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include "shm_wrapper_aux.h"
#include "../logger/logger.h"
#include "../runtime_util/runtime_util.h"     //(TODO: consider removing relative pathnames)

//test process 1 for shm_wrapper_aux. is a dummy net_handler

//sanity gamepad test

//sanity robot description test

//robot description write override test

// *************************************************************************************************** //

void ctrl_c_handler (int sig_num)
{
	printf("Aborting and cleaning up\n");
	fflush(stdout);
	shm_aux_stop(NET_HANDLER);
	logger_stop(NET_HANDLER);
	exit(1);
}

int main()
{
	shm_aux_init(NET_HANDLER);
	logger_init(NET_HANDLER);
	signal(SIGINT, ctrl_c_handler); //hopefully fails gracefully when pressing Ctrl-C in the terminal
	
	sanity_test();
	
	sleep(2);
	
	shm_aux_stop(NET_HANDLER);
	logger_stop(NET_HANDLER);
	
	return 0;
}