#include "shm_client.h"

pid_t shm_pid;

void start_shm ()
{
	//fork shm_start process
	if ((shm_pid = fork()) < 0) {
		printf("fork: %s\n", strerror(errno));
	} else if (shm_pid == 0) { //child
		//cd to the shm_wrapper directory
		if (chdir("../shm_wrapper") == -1) {
			printf("chdir: %s\n", strerror(errno));
		}

		//exec the shm_start process
		if (execlp("./shm_start", "shm", (char *) 0) < 0) {
			printf("execlp: %s\n", strerror(errno));
		}
	} else { //parent
		//wait for shm_start process to terminate
		if (waitpid(shm_pid, NULL, 0) < 0) {
			printf("waitpid shm: %s\n", strerror(errno));
		}
		
		//init to the now-existing shared memory
		shm_init();
	}
}

void stop_shm ()
{
	//fork shm_stop process
	if ((shm_pid = fork()) < 0) {
		printf("fork: %s\n", strerror(errno));
	} else if (shm_pid == 0) { //child
		//cd to the shm_wrapper directory
		if (chdir("../shm_wrapper") == -1) {
			printf("chdir: %s\n", strerror(errno));
		}

		//exec the shm_stop process
		if (execlp("./shm_stop", "shm", (char *) 0) < 0) {
			printf("execlp: %s\n", strerror(errno));
		}
	} else { //parent
		//wait for shm_stop process to terminate
		if (waitpid(shm_pid, NULL, 0) < 0) {
			printf("waitpid shm: %s\n", strerror(errno));
		}
	}
}

void print_shm ()
{
	print_params(~0); //prints all devices
	print_cmd_map();
	print_sub_map();
	print_robot_desc();
	print_gamepad_state();
}
