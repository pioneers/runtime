#include "shm_wrapper.h"

int main (int argc, char *argv[]) 
{
	//prints state of shm to console
	uint32_t devices = 0;
	if (argc == 1) {
		devices = -1; //111111111....
	} else {
		//if provided specific devices to list, construct devices
		for (int i = 1; i < argc; i++) {
			devices |= (1 << atoi(argv[i]));
		}
	}
	
	logger_init(TEST);
	shm_init();
	
	print_params(devices);
	print_cmd_map();
	print_sub_map();
	print_robot_desc();
	print_gamepad_state();
	
}