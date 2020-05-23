#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include "shm_wrapper.h"

//test process 1 for shm_wrapper. is a dummy device handler.
//run this one first, then run process2
//if for some reason you need to quit before the processes terminate (or they're stuck) 
//you'll need to remove the shm file or change the name in shm_wrapper.c

//test the param bitmap and sanity check to make sure shm connection is functioning
void sanity_test ()
{
	param_t params_in[MAX_PARAMS];
	params_in[1].p_i = 10;
	params_in[1].p_f = -0.9;
	params_in[1].p_b = 1;
	
	param_t params_out[MAX_PARAMS];
	
	uint32_t pmap[33];
	
	int dev_ix = -1;
	
	device_connect(0, 3, 382726112, &dev_ix);
	
	print_dev_ids();
	
	printf("Go!\n");
	
	sleep(2); //start up the other process during this time
	
	//process2 writes five times to this block; we read it out and print it
	for (int i = 0; i < 5; i++) {
		//test to see if anything has changed
		while (1) {
			get_param_bitmap(pmap);
			if (pmap[0] != 0) {
				break;
			}
			usleep(1000);
		}
		//read the stuff and print it
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (pmap[0] & (1 << j)) {
				device_read(j, DEV_HANDLER, DOWNSTREAM, pmap[j + 1], params_out);
				print_params(pmap[j + 1], params_out);
			}
		}
	}
	
	sleep(1);
	
	device_disconnect(0);
	
	sleep(2);
}

//test connecting devices in quick succession
//test failure message print when connecting too many devices
//test disconnecting and reconnecting devices on system at capacity
//test disconnecting devices in quick succession
void dev_conn_test ()
{
	int dev_ix = -1;
	
	printf("Beginning device connection test...\n");
	
	//connect as many devices as possible
	for (int i = 0; i < MAX_DEVICES; i++) {
		//randomly chosen quadratic function that is positive and integral in range [0, 32] for the lols
		device_connect(i, i % 3, (-10000 * i * i) + (297493 * i) + 474732, &dev_ix);
	}
	print_dev_ids();
	device_connect(1, 2, 4845873, &dev_ix); //trying to connect one more should error
	sleep(1);
	
	//try and disconnect 3 devices and then adding 3 more devices
	device_disconnect(2);
	device_disconnect(8);
	device_disconnect(7);
	print_dev_ids();
	
	sleep(1);
	
	device_connect(10, 0, 65456874, &dev_ix);
	device_connect(14, 2, 32558656, &dev_ix);
	device_connect(3, 1, 798645325, &dev_ix);
	print_dev_ids();
	
	sleep(1);
	
	//disconnect all devices
	for (int i = 0; i < MAX_DEVICES; i++) {
		device_disconnect(i);
	}
	print_dev_ids();
	
	printf("Done!\n");
}

//test to find approximately how many read/write operations
//can be done in a second on a device downstream block between
//exeuctor and device handler
void single_thread_load_test ()
{
	//writing will be done from process2
	//process1 just needs to read from the block as fast as possible
	int dev_ix = -1;
	int i = 0;
	
	param_t params_out[MAX_PARAMS];
	param_t params_test[MAX_PARAMS];
	uint32_t pmap[MAX_DEVICES + 1];
	
	printf("Beginning single threaded load test...\n");
	
	device_connect(0, 1, 4894249, &dev_ix); //this is going to connect at dev_ix = 0
	device_connect(1, 2, 9848990, &dev_ix); //this is going to connect at dev_ix = 1
	
	//write params_test[0].p_b = 0 into the shared memory block for second device so we don't exit the test prematurely
	params_test[0].p_b = 0; 
	device_write(1, DEV_HANDLER, UPSTREAM, 1, params_test);
	
	//use the second device's p_b on param 0 to indicate when test is done
	//we literally just sit here pulling information from the block as fast as possible 
	//until process2 says we're good
	while (1) {
		//check if time to stop
		i++;
		if (i == 1000) {
			device_read(1, DEV_HANDLER, UPSTREAM, 1, params_test); //use the upstream block to not touch the param bitmap
			if (params_test[0].p_b) {
				break;
			}
			i = 0;
		}
		
		//if something changed, pull it out
		get_param_bitmap(pmap);
		if (pmap[0]) {
			device_read(0, DEV_HANDLER, DOWNSTREAM, pmap[1], params_out);
		}
	}
	
	device_disconnect(1);
	device_disconnect(0);
	
	printf("Done!\n");
}

int main()
{
	shm_init(DEV_HANDLER);
	
	sanity_test();
	
	dev_conn_test();
	
	single_thread_load_test();
	
	sleep(2);
	
	shm_stop(DEV_HANDLER);
	
	return 0;
}