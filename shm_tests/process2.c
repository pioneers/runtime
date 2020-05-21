#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "shm_wrapper.h"

//test process 2 for shm_wrapper

void print_dev_ids ()
{
	dev_id_t dev_ids[MAX_DEVICES];
	
	get_device_identifiers(dev_ids);
	
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (dev_ids[i].uid != 0) {
			printf("dev_ix = %d: type = %d, uid = %llu\n", i, dev_ids[i].type, dev_ids[i].uid);
		}
	}
	printf("\n");
}

void print_params (uint16_t params_to_print, param_t *params)
{
	for (int i = 0; i < MAX_PARAMS; i++) {
		if (params_to_print & (1 << i)) {
			printf("num = %d, p_i = %d, p_f = %f, p_b = %u\n", params[i].num, params[i].p_i, params[i].p_f, params[i].p_b);
		}
	}
}

void print_pmap (uint16_t *pmap)
{
	printf("changed devices: %d\n changed params:", pmap[0]);
	for (int i = 1; i < 17; i++) {
		printf("   %d", pmap[i]);
	}
	printf("\n");
}

int main()
{
	param_t params_in[MAX_PARAMS];
	params_in[1].num = 0;
	params_in[1].p_i = 10;
	params_in[1].p_f = -0.9;
	params_in[1].p_b = 1;
	
	param_t params_out[MAX_PARAMS];
	
	uint16_t pmap[17];
	
	shm_init(EXECUTOR);
	
	printf("hi1\n");
	
	device_update_registry();
	
	printf("hi2\n");
	
	print_dev_ids();
	
	printf("hi3\n");
	
	//write 5 times to the downstream block of the device at varying times
	for (int i = 0; i < 5; i++) {
		params_in[1].p_i += 10;
		params_in[1].p_f += 0.1;
		params_in[1].p_b = 1 - params_in[1].p_b;

		device_write(0, EXECUTOR, DOWNSTREAM, 2, params_in);
		
		get_param_bitmap(pmap);
		print_pmap(pmap);
		
		sleep((unsigned int) ((float) (i + 2)) / 2.0);
	}
	
	get_param_bitmap(pmap);
	print_pmap(pmap);
	
	sleep(5);
	
	shm_stop(EXECUTOR);
	
	return 0;
}