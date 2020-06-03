#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include "shm_wrapper_aux.h"
#include "../logger/logger.h"
#include "../runtime_util/runtime_util.h"     //(TODO: consider removing relative pathnames)

//test process 1 for shm_wrapper_aux. is a dummy net_handler

// *************************************************************************************************** //
void sync ()
{
	uint32_t buttons;
	float joystick_vals[4];

	//write a 1 to a_button
	gamepad_read(&buttons, joystick_vals);
	gamepad_write(buttons | 1, joystick_vals);
	
	//wait for a 1 on b_button
	while (1) {
		gamepad_read(&buttons, joystick_vals);
		if (buttons & 2) {
			break;
		}
		usleep(1000);
	}
	sleep(1);
	gamepad_write(0, joystick_vals);
	sleep(1);
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
	
	buttons = 34788240; //push some random buttons
	joystick_vals[X_LEFT_JOYSTICK] = -0.4854;
	joystick_vals[Y_LEFT_JOYSTICK] = 0.58989;
	joystick_vals[X_RIGHT_JOYSTICK] = 0.9898;
	joystick_vals[Y_RIGHT_JOYSTICK] = -0.776;
	
	gamepad_write(buttons, joystick_vals);
	print_gamepad_state();
	sleep(1);
	
	buttons = 0; //no buttons pushed
	gamepad_write(buttons, joystick_vals);
	print_gamepad_state();
	sleep(1);
	
	buttons = 789597848; //push smoe different random buttons
	joystick_vals[X_LEFT_JOYSTICK] = -0.9489;
	joystick_vals[Y_LEFT_JOYSTICK] = 0.0;
	joystick_vals[X_RIGHT_JOYSTICK] = 1.0;
	joystick_vals[Y_RIGHT_JOYSTICK] = -1.0;
	
	gamepad_write(buttons, joystick_vals);
	print_gamepad_state();
	sleep(1);
	
	buttons = 0;
	for (int i = 0; i < 4; i++) {
		joystick_vals[i] = 0.0;
	}
	gamepad_write(buttons, joystick_vals);
	print_gamepad_state();
	printf("Done!\n\n");
}

// *************************************************************************************************** //
//sanity robot description test
void sanity_robot_desc_test ()
{
	sync();
	
	printf("Begin sanity robot desc test...\n");
	
	for (int i = 0; i < 11; i++) {
		switch (i) {
			case 0:
				robot_desc_write(RUN_MODE, AUTO);
				break;
			case 1:
				robot_desc_write(RUN_MODE, TELEOP);
				break;
			case 2:
				robot_desc_write(DAWN, CONNECTED);
				break;
			case 3:
				robot_desc_write(DAWN, DISCONNECTED);
				break;
			case 4:
				robot_desc_write(SHEPHERD, CONNECTED);
				break;
			case 5:
				robot_desc_write(SHEPHERD, DISCONNECTED);
				break;
			case 6:
				robot_desc_write(GAMEPAD, DISCONNECTED);
				break;
			case 7:
				robot_desc_write(GAMEPAD, CONNECTED);
				break;
			case 8:
				robot_desc_write(TEAM, GOLD);
				break;
			case 9:
				robot_desc_write(TEAM, BLUE);
				break;
			case 10:
				robot_desc_write(RUN_MODE, IDLE);
				break;
		}
		usleep(200000); //sleep for 0.2 sec
	}
	printf("Done!\n\n");
}

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
	robot_desc_write(GAMEPAD, CONNECTED);
	print_robot_desc();
	
	sanity_gamepad_test();
	
	sanity_robot_desc_test();
	
	sleep(2);
	
	shm_aux_stop(NET_HANDLER);
	logger_stop(NET_HANDLER);
	
	return 0;
}