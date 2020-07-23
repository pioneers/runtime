#include <signal.h>
#include "shm_wrapper.h"

void ctrl_c_handler (int sig_num)
{
	printf("Aborting and cleaning up\n");
	fflush(stdout);
	for (int i = 0; i < MAX_DEVICES; i++) {
		device_disconnect(i);
	}
	exit(0);
}

void device_connect_fields (uint16_t type, uint8_t year, uint64_t uid)
{
	int dev_ix;
	dev_id_t dev_id = { type, year, uid };
	device_connect(dev_id, &dev_ix);
}

int main()
{
	param_val_t params_in[MAX_PARAMS];

	logger_init(DEV_HANDLER);
	shm_init();
	signal(SIGINT, ctrl_c_handler); //hopefully fails gracefully when pressing Ctrl-C in the terminal
	//connect as many devices as possible
	for (int i = 0; i < DEVICES_LENGTH; i++) {
		if (i == 6 || i == 8 || i == 9) {
			continue;
		}
		//randomly chosen quadratic function that is positive and integral in range [0, 32] for the lols
		device_connect_fields(i % 13, i % 3, (-10000 * i * i) + (297493 * i) + 474732);
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
