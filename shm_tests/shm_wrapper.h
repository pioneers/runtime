#ifndef SHM_WRAPPER_H
#define SHM_WRAPPER_H

#include <stdio.h>
#include <stdlib.h> //malloc, atoi, etc.
#include <stdint.h> //for integer sizes
#include <string.h> //string manipulation
#include <errno.h> //error handling functions (useful for printing errors)
#include <unistd.h> //standard symobolic constants, utility functions (i.e. sleep)

#include <sys/shm.h> //shm functions and constants 
#include <sys/ipc.h> //for IPC_CREAT and other constants like that
#include <sys/types.h> //time_t, size_t, ssize_t, pthread types, ipc types
#include <sempahore.h> //semaphore functions and constants

#define MAX_DEVICES 16

//hold a single param
typedef struct param {
	int num; //param number
	int p_i; //data if int
	float p_f; //data if float
	bool p_b; //data if bool
} param_t;

void shm_init ();

void shm_close ();

int device_create (uint16_t dev_type, uint64_t uid); //will return the dev_ix for new device, or -1 on failure

void device_write (int dev_ix, uint16_t params_to_write, param_t *params);

void device_read (int dev_ix, uint16_t params_to_read, param_t *params);

void device_close (int dev_ix);

//return value = number of devices with changed params
//pass in an array of uint16_t's that is 17 elements long
//index 0 will be a bitmap of which devices have changed params
//indices 1 - 16 will be a bitmap of which params for those devices have changed
//for those params that have changed, retrieve updated value by calling device_read()
int get_updated_params (uint16_t *update_info); 

#endif