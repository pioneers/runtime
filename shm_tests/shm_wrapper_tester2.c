#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include "shm_wrapper.h"

//test process 2 for shm_wrapper. is a dummy executor

//test the param bitmap and sanity check to make sure shm connection is functioning
void sanity_test()
{
	param_t params_in[MAX_PARAMS];
	params_in[1].p_i = 10;
	params_in[1].p_f = -0.9;
	params_in[1].p_b = 1;
	
	param_t params_out[MAX_PARAMS];
	
	uint32_t pmap[33];
	
	print_dev_ids();
	
	//write 5 times to the downstream block of the device at varying times
	for (int i = 0; i < 5; i++) {
		params_in[1].p_i += 10;
		params_in[1].p_f += 0.1;
		params_in[1].p_b = 1 - params_in[1].p_b;

		device_write(0, EXECUTOR, DOWNSTREAM, 2, params_in);
		
		print_pmap();
		
		sleep((unsigned int) ((float) (i + 2)) / 2.0);
	}
	
	print_pmap();
	
	sleep(1);
}

//test connecting devices in quick succession
//test failure message print when connecting too many devices
//test disconnecting and reconnecting devices on system at capacity
//test disconnecting devices in quick succession
void dev_conn_test ()
{
	//process2's job is simply to monitor what's going on in the catalog
	//as devices connect and disconnect. all the work is being done from process1
	for (int i = 0; i < 7; i++) {
		print_dev_ids();
		usleep(500000); //sleep for 0.5 seconds between prints
	}
}

//test to find approximately how many read/write operations
//can be done in a second on a device downstream block between
//exeuctor and device handler
void single_thread_load_test ()
{
	int dev_ix = -1;
	
	int count = 100; //starting number of writes to complete
	double gain = 40000;
	
	double range = 0.01; //tolerance range of time taken around 1 second for test to complete
	int counts[5]; //to hold the counts that resulted in those times
	int good_trials = 0;
	
	struct timeval start, end;
	double time_taken;
	
	uint32_t catalog = 0;
	uint32_t pmap[MAX_DEVICES + 1];
	param_t params_in[MAX_PARAMS];
	params_in[0].p_b = 0;
	params_in[0].p_i = 1;
	params_in[0].p_f = 2.0;
	
	//wait until the two devices connect
	while (1) {
		get_catalog(&catalog);
		if ((catalog & 1) && (catalog & 2)) {
			break;
		}
	}
	
	//adjust the count until 5 trials lie within 0.01 second of 1 second
	while (good_trials < 5) {
		gettimeofday(&start, NULL);
		for (int i = 0; i < count; i++) {
			//wait until previous write has been pulled out by process1
			while (1) {
				get_param_bitmap(pmap);
				if (!pmap[0]) {
					break;
				}
			}
			device_write(0, EXECUTOR, DOWNSTREAM, 1, params_in); //write into block
		}
		gettimeofday(&end, NULL);
		
		time_taken = (double)(end.tv_sec - start.tv_sec) + ((double)(end.tv_usec - start.tv_usec) / 1000000.0);
		if (time_taken > (1.0 - range) && time_taken < (1.0 + range)) {
			counts[good_trials++] = count;
		} else {
			count += (int)((1.0 - time_taken) * gain);
		}
		printf("count = %d completed in %f seconds for %f writes / second\n", count, time_taken, 1.0 / (time_taken / (double) count));
	}
	
	//manually calculate and print the average bc laziness :P
	printf("\naverage: %f writes / second\n", (double)(counts[0] + counts[1] + counts[2] + counts[3] + counts[4]) / 5.0);
	
	//tell process1 to stop reading
	params_in[0].p_b = 1;
	device_write(1, EXECUTOR, UPSTREAM, 1, params_in);
}

int main()
{	
	shm_init(EXECUTOR);
	
	sanity_test();	
	
	dev_conn_test();
	
	single_thread_load_test();
	
	shm_stop(EXECUTOR);
	
	sleep(2);
	
	return 0;
}