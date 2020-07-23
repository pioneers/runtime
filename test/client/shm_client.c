#include "shm_client.h"

pid_t shm_pid;

void start_shm ()
{
	//fork shm process
	if ((shm_pid = fork()) < 0) {
		printf("fork: %s\n", strerror(errno));
	} else if (shm_pid == 0) { //child
		//exec the actual net_handler process
		if (execlp("../../shm_wrapper/shm", "shm", (char *) 0) < 0) {
			printf("execlp: %s\n", strerror(errno));
		}
	} else { //parent
		sleep(1); //allows shm to set itself up
		shm_init();
	}
}

void stop_shm ()
{
	//send signal to shm process and wait for termination
	if (kill(shm_pid, SIGINT) < 0) {
		printf("kill: %s\n", strerror(errno));
	}
	if (waitpid(shm_pid, NULL, 0) < 0) {
		printf("waitpid: %s\n", strerror(errno));
	}
}

void print_shm ()
{	
	print_params((uint32_t) ~0); //prints all devices
	print_cmd_map();
	print_sub_map();
	print_robot_desc();
	print_gamepad_state();
}