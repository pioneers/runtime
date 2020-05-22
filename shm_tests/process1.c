#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "shm_wrapper.h"

//test process 1 for shm_wrapper

void print_dev_ids ()
{
	dev_id_t dev_ids[MAX_DEVICES];
	uint32_t catalog;
	
	get_device_identifiers(dev_ids);
	get_catalog(&catalog);
	
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (catalog & (1 << i)) {
			printf("dev_ix = %d: type = %d, year = %d, uid = %llu\n", i, dev_ids[i].type, dev_ids[i].year, dev_ids[i].uid);
		}
	}
	printf("\n");
}

void print_params (uint32_t params_to_print, param_t *params)
{
	for (int i = 0; i < MAX_PARAMS; i++) {
		if (params_to_print & (1 << i)) {
			printf("num = %d, p_i = %d, p_f = %f, p_b = %u\n", params[i].num, params[i].p_i, params[i].p_f, params[i].p_b);
		}
	}
}

void print_pmap (uint32_t *pmap)
{
	printf("changed devices: %d\n changed params:", pmap[0]);
	for (int i = 1; i < 33; i++) {
		printf(" %d", pmap[i]);
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
	
	uint32_t pmap[33];
	
	int dev_ix = -1;
	
	shm_init(DEV_HANDLER);
	
	device_connect(0, 3, 382726112, &dev_ix);
	
	print_dev_ids();
	
	printf("Go!\n");
	
	sleep(2); //start up the other process during this time
	
	for (int i = 0; i < 5; i++) {
		while (1) {
			get_param_bitmap(pmap);
			if (pmap[0] != 0) {
				break;
			}
			usleep(1000);
		}
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (pmap[0] & (1 << j)) {
				device_read(j, DEV_HANDLER, DOWNSTREAM, pmap[j + 1], params_out);
				print_params(pmap[j + 1], params_out);
			}
		}
	}
	
	sleep(1);
	
	device_disconnect(0);
	
	shm_stop(DEV_HANDLER);
	
	return 0;
}