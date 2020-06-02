#include <signal.h>
#include "shm_wrapper.h"

void ctrl_c_handler (int sig_num)
{
	printf("Aborting and cleaning up\n");
	fflush(stdout);
	shm_stop(DEV_HANDLER);
	logger_stop(DEV_HANDLER);
	exit(0);
}

int main()
{
	int dev_ix = -1;
	param_val_t params_in[MAX_PARAMS];
	
	shm_init(DEV_HANDLER);
	logger_init(DEV_HANDLER);
	signal(SIGINT, ctrl_c_handler); //hopefully fails gracefully when pressing Ctrl-C in the terminal

	//connect as many devices as possible
	for (int i = 0; i < MAX_DEVICES; i++) {
		//randomly chosen quadratic function that is positive and integral in range [0, 32] for the lols
		device_connect(i, i % 3, (-10000 * i * i) + (297493 * i) + 474732, &dev_ix);
		for (int j = 0; j < MAX_PARAMS; j++) {
			params_in[j].p_i = j * i * MAX_DEVICES;
			params_in[j].p_f = (float)(j * i * MAX_DEVICES * 3.14159);
			params_in[j].p_b = (i % 2 == 0) ? 0 : 1;
		}
		device_write(i, DEV_HANDLER, DATA, 4294967295, params_in);
	}
	print_dev_ids();
	
	while (1) {
		
		sleep(1000);
	}
	
	return 0;
}