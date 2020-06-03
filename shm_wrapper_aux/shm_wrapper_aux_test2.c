#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include "shm_wrapper_aux.h"
#include "../logger/logger.h"
#include "../runtime_util/runtime_util.h"     //(TODO: consider removing relative pathnames)

//test process 2 for shm_wrapper_aux. is a dummy executor

// *************************************************************************************************** //
void sync ()
{
	uint32_t buttons;
	float joystick_vals[4];
	
	//write a 1 to b_button
	gamepad_read(&buttons, joystick_vals);
	gamepad_write(buttons | 2, joystick_vals);
	
	//wait on a 1 to a_button
	while (1) {
		gamepad_read(&buttons, joystick_vals);
		if (buttons & 1) {
			break;
		}
		usleep(1000);
	}
	sleep(2);
	printf("\tSynced; starting test!\n");
}

// *************************************************************************************************** //
//sanity gamepad test
void sanity_gamepad_test ()
{
	uint32_t buttons;
	float joystick_vals[4];
	
	sync();
	
	printf("Begin sanity gamepad test...\n");
	
	for (int i = 0; i < 7; i++) {
		gamepad_read(&buttons, joystick_vals);
		printf("buttons = %d\t joystick_vals = (", buttons);
		for (int j = 0; j < 4; j++) {
			printf("%f, ", joystick_vals[j]);
		}
		printf(")\n");
		usleep(500000); //sleep for half a second
	}
	printf("Done!\n\n");
}

// *************************************************************************************************** //
//sanity robot description test
void sanity_robot_desc_test ()
{
	int count = 0;
	robot_desc_val_t curr[6] = { IDLE, DISCONNECTED, DISCONNECTED, CONNECTED, BLUE };
	robot_desc_val_t prev[6] = { IDLE, DISCONNECTED, DISCONNECTED, CONNECTED, BLUE };
	
	sync();
	
	printf("Begin sanity robot desc test...\n");
	
	while (count < 11) {
		for (int i = 0; i < NUM_DESC_FIELDS; i++) {
			curr[i] = robot_desc_read(i);
			if (curr[i] != prev[i]) {
				printf("something has changed! new robot description:\n");
				print_robot_desc();
				prev[i] = curr[i];
				count++;
			}
		}
		usleep(100);
	}
	printf("Done!\n\n");
}

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
	
	sanity_gamepad_test();
	
	sanity_robot_desc_test();
	
	shm_aux_stop(EXECUTOR);
	logger_stop(EXECUTOR);
	
	sleep(2);
	
	return 0;
}