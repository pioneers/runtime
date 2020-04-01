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

#define MAX_DEVICES 16 //maximum number of connected devices
#define MAX_PARAMS 16 //maximum number of parameters per connected device (probably should be defined elsewhere)

//names for the two associated blocks per device
#define UPSTREAM 0
#define DOWNSTREAM 1

//names for possible calling processes
#define DEV_HANDLER 0
#define EXECUTOR 1
#define NET_HANDLER 2

//hold a single param
typedef struct param {
	int num; //param number
	int p_i; //data if int
	float p_f; //data if float
	uint8_t p_b; //data if bool
} param_t;

//holds the device identification information of a single device
typedef struct dev_id {
	uint16_t type;
	uint64_t uid;
} dev_id_t;

/*
Call this function from every process that wants to use the shared memory wrapper
Should be called before any other action happens
The device handler process is responsible for initializing the catalog and updated param bitmap
	- process: one of DEV_HANDLER, EXECUTOR, NET_HANDLER to specify which process this function is being
		called from
Returns 0 on success, various numbers from 1 - 4 on various errors (prints to stderr)
*/
int shm_init (uint8_t process);

/*
Call this function if process no longer wishes to connect to shared memory wrapper
No other actions will work after this function is called
Device handler is responsible for marking shared memory for destruction after all detach
	- process: one of DEV_HANDLER, EXECUTOR, NET_HANDLER to specify which process this function is being
		called from
Returns 0 on success, various numbers from 1 - 7 on various errors (prints to stderr)
*/
int shm_stop (uint8_t process);

/*
Should only be called from device handler
Finds a place to put the device data shared memory blocks and sets it up
	- dev_type: the type of the device being connected e.g. LIMITSWITCH, LINEFOLLOWER, etc; see device_config.h
	- dev_uid: the unique, random 64-bit uid assigned to the device when flashing
Returns 0 on success, various numbers from 1 - 4 for various errors (prints to stderr)
*/
int device_connect (uint16_t dev_type, uint64_t dev_uid);

/*
Should only be called from device handler
Disconnects a device with a given index by closing mappings to all associated shared memory
and unlinking associated semaphores. Also updates the catalog bitmap
	- dev_ix: index of device in catalog to be disconnected
Returns 0 on success, various numbers from 1 - 6 for various errors (prints to stderr)
*/
int device_disconnect (int dev_ix);

/*
Should be called periodically by every process except the device handler
Compares local and shared memory copy of the valid device indices and determines if any devices have
recently connected / disconnected. If so, then takes appropriate action to update device registry
for this process. Updates local catalog.
Returns 0 on success, various numbers from 1 - 7 for various errors (prints to stderr)
*/
int device_update_registry ();

/*	
Should be called from every process wanting to read the device data
Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
	- dev_ix: device index of the device whose data is being requested
	- process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
	- stream: the requested block to read from, one of UPSTREAM, DOWNSTREAM
	- params_to_read: bitmap representing which params to be read 
		(nonexistent params should have corresponding bits set to 0)
	- params: pointer to array of param_t's that is at least as long as highest requested param number
		device data will be read into the corresponding param_t's
No return value.
*/
void device_read (int dev_ix, uint8_t process, uint8_t stream, uint16_t params_to_read, param_t *params);

/*	
Should be called from every process wanting to write to the device data
Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
	- dev_ix: device index of the device whose data is being written
	- process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
	- stream: the requested block to write to, one of UPSTREAM, DOWNSTREAM
	- params_to_read: bitmap representing which params to be written
		(nonexistent params should have corresponding bits set to 0)
	- params: pointer to array of param_t's that is at least as long as highest requested param number
		device data will be written into the corresponding param_t's
No return value.
*/
void device_write (int dev_ix, uint8_t process, uint8_t stream, uint16_t params_to_write, param_t *params);

/*
Should be called from all processes that want to know current state of the param bitmap (i.e. device handler)
	- bitmap: pointer to array of 17 16-bit integers to copy the bitmap into
No return value.
*/
void get_param_bitmap (uint16_t *bitmap);

/*
Should be called from all processes that want to know device identifiers of all currently connected devices
	- dev_ids: pointer to array of dev_id_t's to copy the information into
No return value.
*/
void get_device_identifiers (dev_id_t *dev_ids);

#endif