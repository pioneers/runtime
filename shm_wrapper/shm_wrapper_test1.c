#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include "shm_wrapper.h"

//test process 1 for shm_wrapper. is a dummy device handler.
//run this one first, then run process2
//if for some reason you need to quit before the processes terminate (or they're stuck) 
//you'll need to remove the shm file or change the name in shm_wrapper.c

// ************************************* HELPER FUNCTIONS ******************************************** //

void sync()
{
	int dev_ix;
	param_val_t params_in[MAX_PARAMS];
	param_val_t params_out[MAX_PARAMS];
	
	device_connect(0, 0, 48482, &dev_ix); //this will connect in dev_ix = 0
	params_in[0].p_b = 0;
	device_write(0, DEV_HANDLER, DATA, 1, params_in); //write a 0 to the upstream block
	params_in[0].p_b = 1;
	while (1) {
		device_write(0, DEV_HANDLER, COMMAND, 1, params_in); //write 1 to the downstream block
		device_read(0, DEV_HANDLER, DATA, 1, params_out); //wait on a 1 in the upstream block
		if (params_out[0].p_b) {
			break;
		}
		usleep(100);
	}
	sleep(1);
	device_disconnect(0);
	sleep(1);
	printf("\tSynced; starting test!\n");
}

// *************************************************************************************************** //
//test the param bitmap and sanity check to make sure shm connection is functioning
void sanity_test ()
{
	param_val_t params_in[MAX_PARAMS];
	params_in[1].p_i = 10;
	params_in[1].p_f = -0.9;
	params_in[1].p_b = 1;
	
	param_val_t params_out[MAX_PARAMS];
	
	uint32_t pmap[33];
	
	int dev_ix = -1;
	
	sync();
	
	device_connect(0, 3, 382726112, &dev_ix);
	
	print_dev_ids();
	
	printf("Beginning sanity test...\n");
	
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
				device_read(j, DEV_HANDLER, COMMAND, pmap[j + 1], params_out);
				print_params(pmap[j + 1], params_out);
			}
		}
	}
	
	sleep(1);
	
	device_disconnect(0);
	
	printf("Done!\n");
}

// *************************************************************************************************** //
//test connecting devices in quick succession
//test failure message print when connecting too many devices
//test disconnecting and reconnecting devices on system at capacity
//test disconnecting devices in quick succession
void dev_conn_test ()
{
	int dev_ix = -1;
	
	sync();
	
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
	
	printf("Done!\n\n");
}

// *************************************************************************************************** //
//test to find approximately how many read/write operations
//can be done in a second on a device downstream block between
//exeuctor and device handler
void single_thread_load_test ()
{
	//writing will be done from process2
	//process1 just needs to read from the block as fast as possible
	int dev_ix = -1;
	int i = 0;
	
	param_val_t params_out[MAX_PARAMS];
	param_val_t params_test[MAX_PARAMS];
	uint32_t pmap[MAX_DEVICES + 1];
	
	sync();
	
	printf("Beginning single threaded load test...\n");
	
	device_connect(0, 1, 4894249, &dev_ix); //this is going to connect at dev_ix = 0
	device_connect(1, 2, 9848990, &dev_ix); //this is going to connect at dev_ix = 1
	
	//write params_test[0].p_b = 0 into the shared memory block for second device so we don't exit the test prematurely
	params_test[0].p_b = 0; 
	device_write(1, DEV_HANDLER, DATA, 1, params_test);
	
	//use the second device's p_b on param 0 to indicate when test is done
	//we literally just sit here pulling information from the block as fast as possible 
	//until process2 says we're good
	while (1) {
		//check if time to stop
		i++;
		if (i == 1000) {
			device_read(1, DEV_HANDLER, DATA, 1, params_test); //use the upstream block to not touch the param bitmap
			if (params_test[0].p_b) {
				break;
			}
			i = 0;
		}
		
		//if something changed, pull it out
		get_param_bitmap(pmap);
		if (pmap[0]) {
			device_read(0, DEV_HANDLER, COMMAND, pmap[1], params_out);
		}
	}
	
	device_disconnect(1);
	device_disconnect(0);
	
	sleep(1);
	
	printf("Done!\n");
}

// *************************************************************************************************** //
//threaded function for reading in Dual Thread Read Write Test
void *read_thread_dtrwt (void *arg)
{
	int prev_val = 0, count = 0, i = 0;
	param_val_t params_test[MAX_PARAMS];
	param_val_t params_out[MAX_PARAMS];
	uint32_t pmap[MAX_DEVICES + 1];
	
	//we are reading from the device command block
	//use the device command block on device 1 so that tester2 can signal end of test
	params_test[0].p_b = 0; 
	device_write(1, DEV_HANDLER, COMMAND, 1, params_test); 
	
	//use the second device device's p_b on param 0 to indicate when test is done 
	//sit here pulling information from the block as fast as possible, 
	//and count how many unique values we get until tester2 says we're done 
	while (1) {
		//check if time to stop 
		i++;
		if (i == 1000) {
			device_read(1, DEV_HANDLER, COMMAND, 1, params_test);
			if (params_test[0].p_b) {
				break;
			}
			i = 0;
		}
		
		//if something changed, pull it out and record it
		get_param_bitmap(pmap);
		if (pmap[0]) {
			device_read(0, DEV_HANDLER, COMMAND, pmap[1], params_out);
			if (prev_val != params_out[0].p_i) {
				prev_val = params_out[0].p_i;
				count++;
			}
		}
	}
	printf("read_thread from tester1 pulled %d unique values from tester2 on downstream block\n", count);
	
	return NULL;
}

//threaded function for writing in Dual Thread Read Write Test
void *write_thread_dtrwt (void *arg)
{
	const int trials = 100000; //write 100000 times to the block
	param_val_t params_test[MAX_PARAMS];
	param_val_t params_in[MAX_PARAMS];
	int x;
	
	//we are writing to the device data block
	//write 100,000 times on the device upstream block as fast as possible
	params_in[0].p_i = 1;
	for (int i = 0; i < trials; i++) {
		(params_in[0].p_i)++;
		device_write(0, DEV_HANDLER, DATA, 1, params_in);
	}
	printf("write_thread from tester1 wrote %d values to upstream block\n", trials);
	
	//signal on the device data block on device 1 so tester2 can stop reading
	params_test[0].p_b = 1;
	device_write(1, DEV_HANDLER, DATA, 1, params_test);
	
	return NULL;
}

//test reading and writing on a device's upstream and downstream block
//at the same time using two threads. from the device handler, we will be
//writing to the upstream block and reading from the downstream block
void dual_thread_read_write_test ()
{
	int dev_ix = -1;
	int status;
	pthread_t read_tid, write_tid; //thread_ids for the two threads
	
	sync();
	
	printf("Beginning dual thread read write test...\n");
	
	device_connect(0, 1, 4894249, &dev_ix);
	device_connect(1, 2, 3556498, &dev_ix);
	
	//create threads
	if ((status = pthread_create(&read_tid, NULL, read_thread_dtrwt, NULL)) != 0) {
		printf("read pthread creation failed with exit code %d\n", status);
	}
	printf("Read thread created\n");
	
	if ((status = pthread_create(&write_tid, NULL, write_thread_dtrwt, NULL)) != 0) {
		printf("write pthread creation failed with exit code %d\n", status);
	}
	printf("Write thread created\n");
	
	//wait for the threads to finish
	pthread_join(read_tid, NULL);
	pthread_join(write_tid, NULL);
	
	device_disconnect(0);
	device_disconnect(1);
	
	sleep(1);
	
	printf("Done!\n");
}

// *************************************************************************************************** //
//test to find approximately how many read/write operations
//can be done in a second on a device downstream block between
//exeuctor and device handler USING DEV_UID READ/WRITE FUNCTIONS
void single_thread_load_test_uid ()
{
	//writing will be done from process2
	//process1 just needs to read from the block as fast as possible
	int dev_ix = -1;
	int dev_uid = 0;
	int i = 0;
	
	param_val_t params_out[MAX_PARAMS];
	param_val_t params_test[MAX_PARAMS];
	uint32_t pmap[MAX_DEVICES + 1];
	
	sync();
	
	printf("Beginning single threaded load test using UID functions...\n");
	
	dev_uid = 4894249;
	
	device_connect(0, 1, dev_uid, &dev_ix); //this is going to connect at dev_ix = 0
	device_connect(1, 2, 9848990, &dev_ix); //this is going to connect at dev_ix = 1
	
	//write params_test[0].p_b = 0 into the shared memory block for second device so we don't exit the test prematurely
	params_test[0].p_b = 0; 
	device_write(1, DEV_HANDLER, DATA, 1, params_test);
	
	//use the second device's p_b on param 0 to indicate when test is done
	//we literally just sit here pulling information from the block as fast as possible 
	//until process2 says we're good
	while (1) {
		//check if time to stop
		i++;
		if (i == 1000) {
			device_read(1, DEV_HANDLER, DATA, 1, params_test); //use the upstream block to not touch the param bitmap
			if (params_test[0].p_b) {
				break;
			}
			i = 0;
		}
		
		//if something changed, pull it out
		get_param_bitmap(pmap);
		if (pmap[0]) {
			device_read_uid(dev_uid, DEV_HANDLER, COMMAND, pmap[1], params_out);
		}
	}
	
	device_disconnect(1);
	device_disconnect(0);
	
	sleep(1);
	
	printf("Done!\n");
}

// *************************************************************************************************** //
void ctrl_c_handler (int sig_num)
{
	printf("Aborting and cleaning up\n");
	fflush(stdout);
	shm_stop(DEV_HANDLER);
	logger_stop(DEV_HANDLER);
	exit(1);
}

int main()
{
	shm_init(DEV_HANDLER);
	logger_init(DEV_HANDLER);
	signal(SIGINT, ctrl_c_handler); //hopefully fails gracefully when pressing Ctrl-C in the terminal
	
	sanity_test();
	
	dev_conn_test();
	
	single_thread_load_test();
	
	dual_thread_read_write_test();
	
	single_thread_load_test_uid();
	
	sleep(2);
	
	shm_stop(DEV_HANDLER);
	logger_stop(DEV_HANDLER);
	
	return 0;
}