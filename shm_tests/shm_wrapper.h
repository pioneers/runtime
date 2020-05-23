#ifndef SHM_WRAPPER_H
#define SHM_WRAPPER_H

#include <stdio.h>          //for i/o
#include <stdlib.h>         //for standard utility functions (exit, sleep)
#include <sys/types.h>      //for sem_t and other standard system types
#include <sys/stat.h>       //for some of the flags that are used (the mode constants)
#include <fcntl.h>          //for flags used for opening and closing files (O_* constants)
#include <unistd.h>         //for standard symbolic constants
#include <semaphore.h>      //for semaphores
#include <sys/mman.h>       //for posix shared memory

#define MAX_DEVICES 32 //maximum number of connected devices
#define MAX_PARAMS 32 //maximum number of parameters per connected device (probably should be defined elsewhere)

//names for the two associated blocks per device
#define UPSTREAM 0
#define DOWNSTREAM 1

//names for possible calling processes
#define DEV_HANDLER 0
#define EXECUTOR 1
#define NET_HANDLER 2

//hold a single param
typedef struct param {
	int p_i;       //data if int
	float p_f;     //data if float
	uint8_t p_b;   //data if bool
} param_t;

//holds the device identification information of a single device
typedef struct dev_id {
	uint16_t type;
	uint8_t year;
	uint64_t uid;
} dev_id_t;

// ******************************************* UTILITY FUNCTIONS ****************************************** //

void print_pmap ();

void print_dev_ids ();	

void print_catalog ();

void print_params (uint32_t params_to_print, param_t *params);

// ******************************************* WRAPPER FUNCTIONS ****************************************** //

/*
Call this function from every process that wants to use the shared memory wrapper
Should be called before any other action happens
The device handler process is responsible for initializing the catalog and updated param bitmap
	- process: one of DEV_HANDLER, EXECUTOR, NET_HANDLER to specify which process this function is being
		called from
No return value.
*/
void shm_init (uint8_t process);

/*
Call this function if process no longer wishes to connect to shared memory wrapper
No other actions will work after this function is called
Device handler is responsible for marking shared memory block and semaphores for destruction after all detach
	- process: one of DEV_HANDLER, EXECUTOR, NET_HANDLER to specify which process this function is being
		called from
No return value.
*/
void shm_stop (uint8_t process);

/*
Should only be called from device handler
Selects the next available device index and assigns the newly connected device to that location. 
Turns on the associated bit in the catalog.
	- dev_type: the type of the device being connected e.g. LIMITSWITCH, LINEFOLLOWER, etc; see device_config.h
	- dev_year: year of device manufacture
	- dev_uid: the unique, random 64-bit uid assigned to the device when flashing
	- dev_ix: the index that the device was assigned will be put here
No return value.
*/
void device_connect (uint16_t dev_type, uint8_t dev_year, uint64_t dev_uid, int *dev_ix);

/*
Should only be called from device handler
Disconnects a device with a given index by turning off the associated bit in the catalog.
	- dev_ix: index of device in catalog to be disconnected
No return value.
*/
void device_disconnect (int dev_ix);

/*	
Should be called from every process wanting to read the device data
Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
Grabs either one or two semaphores depending on calling process and stream requested.
	- dev_ix: device index of the device whose data is being requested
	- process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
	- stream: the requested block to read from, one of UPSTREAM, DOWNSTREAM
	- params_to_read: bitmap representing which params to be read 
		(nonexistent params should have corresponding bits set to 0)
	- params: pointer to array of param_t's that is at least as long as highest requested param number
		device data will be read into the corresponding param_t's
No return value.
*/
void device_read (int dev_ix, uint8_t process, uint8_t stream, uint32_t params_to_read, param_t *params);

/*	
Should be called from every process wanting to write to the device data
Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
Grabs either one or two semaphores depending on calling process and stream requested.
	- dev_ix: device index of the device whose data is being written
	- process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
	- stream: the requested block to write to, one of UPSTREAM, DOWNSTREAM
	- params_to_read: bitmap representing which params to be written
		(nonexistent params should have corresponding bits set to 0)
	- params: pointer to array of param_t's that is at least as long as highest requested param number
		device data will be written into the corresponding param_t's
No return value.
*/
void device_write (int dev_ix, uint8_t process, uint8_t stream, uint32_t params_to_write, param_t *params);

/*
Should be called from all processes that want to know current state of the param bitmap (i.e. device handler)
Blocks on the param bitmap semaphore for obvious reasons
	- bitmap: pointer to array of 17 32-bit integers to copy the bitmap into
No return value.
*/
void get_param_bitmap (uint32_t *bitmap);

/*
Should be called from all processes that want to know device identifiers of all currently connected devices
Blocks on catalog semaphore for obvious reasons
	- dev_ids: pointer to array of dev_id_t's to copy the information into
No return value.
*/
void get_device_identifiers (dev_id_t *dev_ids);

/*
Should be called from all processes that want to know which dev_ix's are valid
Blocks on catalog semaphore for obvious reasons
	- catalog: pointer to 32-bit integer into which the current catalog will be read into
No return value.
*/
void get_catalog (uint32_t *catalog);

#endif