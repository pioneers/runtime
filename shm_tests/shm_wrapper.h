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

#define UPSTREAM 0
#define DOWNSTREAM 1

#define DEV_HANDLER 0
#define EXECUTOR 1
#define NET_HANDLER 2

//hold a single param
typedef struct param {
	int num; //param number
	int p_i; //data if int
	float p_f; //data if float
	bool p_b; //data if bool
} param_t;

//holds the device identification information of a single device
typedef struct dev_id {
	uint16_t type;
	uint64_t uid;
} dev_id_t;

//takes care of initialization of the shm wrapper facilities for a process
int shm_init (uint8_t process);

//takes care of closing of the shm wrapper facilities for a process
int shm_stop (uint8_t process);

void device_connect (uint16_t dev_type, uint64_t dev_uid);

void device_disconnect (int dev_ix);

void device_update_registry ();

void device_read (int dev_ix, uint8_t stream, uint16_t params_to_read, param_t *params);

void device_write (int dev_ix, uint8_t stream, uint16_t params_to_write, param_t *params);

void device_close (int dev_ix);

//return value = number of devices with changed params
//pass in an array of uint16_t's that is 17 elements long
//index 0 will be a bitmap of which devices have changed params
//indices 1 - 16 will be a bitmap of which params for those devices have changed
//for those params that have changed, retrieve updated value by calling device_read()
void get_param_bitmap (uint16_t *bitmap);

void get_device_identifiers (dev_id_t **dev_ids);

#endif