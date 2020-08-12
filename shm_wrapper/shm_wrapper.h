#ifndef SHM_WRAPPER_H
#define SHM_WRAPPER_H

#include <stdio.h>                         //for i/o
#include <stdlib.h>                        //for standard utility functions (exit, sleep)
#include <stdint.h>                        //for standard integer types
#include <sys/types.h>                     //for sem_t and other standard system types
#include <sys/stat.h>                      //for some of the flags that are used (the mode constants)
#include <fcntl.h>                         //for flags used for opening and closing files (O_* constants)
#include <unistd.h>                        //for standard symbolic constants
#include <signal.h>						   // for signal handlers
#include <semaphore.h>                     //for semaphores
#include <sys/mman.h>                      //for posix shared memory
#include <limits.h>						   //for UCHAR_MAX
#include "../logger/logger.h"              //for logger
#include "../runtime_util/runtime_util.h"  //for runtime constants


//names of various objects used in shm_wrapper; should not be used outside of shm_wrapper and shm.c
#define CATALOG_MUTEX_NAME "/ct-mutex"  //name of semaphore used as a mutex on the catalog
#define CMDMAP_MUTEX_NAME "/cmap-mutex" //name of semaphore used as a mutex on the command bitmap
#define SUBMAP_MUTEX_NAME "/smap-mutex" //name of semaphore used as a mutex on the various subcription bitmaps
#define DEV_SHM_NAME "/dev-shm"         //name of shared memory block across devices

#define GPAD_SHM_NAME "/gp-shm"         //name of shared memory block for gamepad
#define ROBOT_DESC_SHM_NAME "/rd-shm"   //name of shared memory block for robot description
#define GP_MUTEX_NAME "/gp-sem"         //name of semaphore used as mutex over gamepad shm
#define RD_MUTEX_NAME "/rd-sem"         //name of semaphore used as mutex over robot description shm

#define LOG_DATA_SHM "/log-data-shm"
#define LOG_DATA_MUTEX "/log-data-sem"

#define SNAME_SIZE 32 //size of buffers that hold semaphore names, in bytes

// *********************************** SHM TYPEDEFS  ****************************************************** //

//enumerated names for the two associated blocks per device
typedef enum stream { DATA, COMMAND } stream_t;

//shared memory has these parts in it
typedef struct dev_shm {
	uint32_t catalog;                                   //catalog of valid devices
	uint32_t cmd_map[MAX_DEVICES + 1];                  //bitmap is 33 32-bit integers (changed devices and changed params of device commands from executor to dev_handler)
	uint32_t net_sub_map[MAX_DEVICES + 1];              //bitmap is 33 32-bit integers (changed devices and changed params in which data net_handler is subscribed to)
	uint32_t exec_sub_map[MAX_DEVICES + 1];             //bitmap is 33 32-bit integers (changed devices and changed params in which data executor is subscribed to)
	param_val_t params[2][MAX_DEVICES][MAX_PARAMS];     //all the device parameter info, data and commands
	dev_id_t dev_ids[MAX_DEVICES];                      //all the device identification info
} dev_shm_t;

//two mutex semaphores for each device
typedef struct sems {
	sem_t *data_sem;        //semaphore on the data stream of a device
	sem_t *command_sem;     //semaphore on the command stream of a device
} dual_sem_t;

//shared memory for gamepad
typedef struct gp_shm {
	uint32_t buttons;       //bitmap for which buttons are pressed
	float joysticks[4];     //array to hold joystick positions
} gamepad_shm_t;

//shared memory for robot description
typedef struct robot_desc_shm {
	uint8_t fields[NUM_DESC_FIELDS];   //array to hold the robot state (each is a uint8_t)
} robot_desc_shm_t;

typedef struct {
	uint8_t num_params;
	param_val_t params[UCHAR_MAX];
	char names[UCHAR_MAX][64];
	param_type_t types[UCHAR_MAX];
} log_data_shm_t;

// *********************************** SHM EXTERNAL VARIABLES  ******************************************** //

// DO NOT USE THESE UNDER NORMAL CIRCUMSTANCES
// THESE ARE ONLY USED TO SIMPLIFY CODE IN SHM_START AND SHM_STOP

extern dual_sem_t sems[MAX_DEVICES];      //array of semaphores, two for each possible device (one for data and one for commands)
extern dev_shm_t *dev_shm_ptr;            //points to memory-mapped shared memory block for device data and commands
extern sem_t *catalog_sem;                //semaphore used as a mutex on the catalog
extern sem_t *cmd_map_sem;                //semaphore used as a mutex on the command bitmap
extern sem_t *sub_map_sem;                //semaphore used as a mutex on the subscription bitmap

extern gamepad_shm_t *gp_shm_ptr;         //points to memory-mapped shared memory block for gamepad
extern robot_desc_shm_t *rd_shm_ptr;      //points to memory-mapped shared memory block for robot description
extern sem_t *gp_sem;                     //semaphore used as a mutex on the gamepad
extern sem_t *rd_sem;                     //semaphore used as a mutex on the robot description

extern log_data_shm_t *log_data_shm_ptr;  // points to shared memory block for log data specified by executor
extern sem_t *log_data_sem;			      //semaphore used as a mutex on the log data

// ******************************************* PRINTING UTILITIES ***************************************** //

/*
 * Prints the current values in the command bitmap
 */
void print_cmd_map ();

/*
 * Prints the current values in the subscription bitmap
 */
void print_sub_map ();

/*
 * Prints the device identification info of the currently attached devices
 */
void print_dev_ids ();

/*
 * Prints the catalog (i.e. how many and at which indices devices are currently attached)
 */
void print_catalog ();

/*
 * Prints the params of the specified devices
 */
void print_params (uint32_t devices);

/*
 * Prints the current values of the robot description in a human-readable way
 */
void print_robot_desc ();

/*
 * Prints the current state of the gamepad in a human-readable way
 */
void print_gamepad_state ();


/*
 *	Prints the current custom logged data.
 */
void print_custom_data();

// ****************************************** SEMAPHORE UTILITIES ***************************************** //

/*
 * Function that generates a semaphore name for the data and command streams
 * Arguments:
 *    stream: stream for which the semaphore is created (one of DATA or COMMAND)
 *    dev_ix: index of device whose STREAM is being created
 *    name: pointer to a buffer of size SNAME_SIZE into which the name of the semaphore will be put
 */
void generate_sem_name (stream_t stream, int dev_ix, char *name);

/*
 * Custom wrapper function for sem_wait. Prints out descriptive logging message on failure
 * Arguments:
 *    sem: pointer to a semaphore to wait on
 *    sem_desc: string that describes the semaphore being waited on, displayed with error message
 */
void my_sem_wait (sem_t *sem, char *sem_desc);

/*
 * Custom wrapper function for sem_post. Prints out descriptive logging message on failure
 * Arguments:
 *    sem: pointer to a semaphore to post
 *    sem_desc: string that describes the semaphore being posted, displayed with error message
 */
void my_sem_post (sem_t *sem, char *sem_desc);

/*
 * Custom wrapper function for sem_open. Prints out descriptive logging message on failure
 * exits (not being able to open a semaphore is fatal to Runtime).
 * Arguments:
 *    sem_name: name of a semaphore to be opened
 *    sem_desc: string that describes the semaphore being opened, displayed with error message
 * Returns a pointer to the semaphore that was opened.
 */
sem_t *my_sem_open (char *sem_name, char *sem_desc);

/*
 * Custom wrapper function for sem_open with create flag. Prints out descriptive logging message on failure
 * exits (not being able to create and open a semaphore is fatal to Runtime).
 * Arguments:
 *    sem_name: name of a semaphore to be created and opened
 *    sem_desc: string that describes the semaphore being created and opened, displayed with error message
 * Returns a pointer to the semaphore that was created and opened.
 */
sem_t *my_sem_open_create (char *sem_name, char *sem_desc);

/*
 * Custom wrapper function for sem_close. Prints out descriptive logging message on failure
 * Arguments:
 *    sem: pointer to a semaphore to close
 *    sem_desc: string that describes the semaphore being closed, displayed with error message
 */
void my_sem_close (sem_t *sem, char *sem_desc);

// ******************************************* WRAPPER FUNCTIONS ****************************************** //

/*
 * Call this function from every process that wants to use the shared memory wrapper
 * No return value (will exit on fatal errors).
 */
void shm_init ();

/*
 * Call this function if process no longer wishes to connect to shared memory wrapper
 * No other actions will work after this function is called.
 * No return value.
 */
void shm_stop ();

/*
 * Should only be called from device handler
 * Selects the next available device index and assigns the newly connected device to that location.
 * Turns on the associated bit in the catalog.
 * Arguments:
 *    - dev_id_t dev_id: device identification info for the device being connected (type, year, uid)
 *    - int *dev_ix: the index that the device was assigned will be put here
 * Returns device index of connected device in dev_ix on success; sets *dev_ix = -1 on failure
 */
void device_connect (dev_id_t dev_id, int *dev_ix);

/*
 * Should only be called from device handler
 * Disconnects a device with a given index by turning off the associated bit in the catalog.
 * Arguments
 *    - int dev_ix: index of device in catalog to be disconnected
 * No return value.
 */
void device_disconnect (int dev_ix);

/*
 * Should be called from every process wanting to read the device data
 * Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
 * Arguments:
 *    - int dev_ix: device index of the device whose data is being requested
 *    - process_t process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
 *    - stream_t stream: the requested block to read from, one of DATA, COMMAND
 *    - uint32_t params_to_read: bitmap representing which params to be read  (nonexistent params should have corresponding bits set to 0)
 *    - param_val_t params: pointer to array of param_val_t's that is at least as long as highest requested param number
 *            device data will be read into the corresponding param_val_t's
 * Returns 0 on success, -1 on failure (specified device is not connected in shm)
 */
int device_read (int dev_ix, process_t process, stream_t stream, uint32_t params_to_read, param_val_t *params);

/*
 * This function is the exact same as the above function, but instead uses the 64-bit device UID to identify
 * the device that should be read, rather than the device index.
 */
int device_read_uid (uint64_t dev_uid, process_t process, stream_t stream, uint32_t params_to_read, param_val_t *params);

/*
 * Should be called from every process wanting to write to the device data
 * Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
 * Grabs either one or two semaphores depending on calling process and stream requested.
 * Arguments
 *    - int dev_ix: device index of the device whose data is being written
 *    - process_t process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
 *    - stream_t stream: the requested block to write to, one of DATA, COMMAND
 *    - uint32_t params_to_read: bitmap representing which params to be written (nonexistent params should have corresponding bits set to 0)
 *    - param_val_t params: pointer to array of param_val_t's that is at least as long as highest requested param number
 *            device data will be written into the corresponding param_val_t's
 * Returns 0 on success, -1 on failure (specified device is not connected in shm)
 */
int device_write (int dev_ix, process_t process, stream_t stream, uint32_t params_to_write, param_val_t *params);

/*
 * This function is the exact same as the above function, but instead uses the 64-bit device UID to identify
 * the device that should be written, rather than the device index.
 */
int device_write_uid (uint64_t dev_uid, process_t process, stream_t stream, uint32_t params_to_write, param_val_t *params);

/*
 * Send a sub request to dev_handler for a particular device. Takes care of updating the changed bits.
 * Should only be called by executor and net_handler
 * Arguments:
 *    - uint64_t dev_uid: unique 64-bit identifier of the device
 *    - process_t process: the calling process (will error if not EXECUTOR or NET_HANDLER)
 *    - uint32_t params_to_sub: bitmap representing params to subscribe to (nonexistent params should have corresponding bits set to 0)
 * Returns 0 on success, -1 on failure (unrecognized process, or device is not connect in shm)
 */
int place_sub_request (uint64_t dev_uid, process_t process, uint32_t params_to_sub);

/*
 * Get current subscription requests for all devices. Should only be called by dev_handler
 * Arguments:
 *    - uint32_t sub_map[MAX_DEVICES + 1]: bitwise OR of the executor and net_handler sub_maps that will be put into this provided buffer
 *        (expects an array of 33 elements, where the 0th index is a bitmap indicating which devices require a new sub request to be sent,
 *        and the remaining 32 elements indicate what the subscription request to each device should be if there are changes)
 *    - process_t process: If EXECUTOR, fills with executor subscriptions. If NET_HANDLER, fills with net_handler subscriptions. 
 *                         If DEV_HANDLER, fills with subscriptions of both and also manages the bitmap. Else, fills with subscriptions of both.
 * No return value.
 */
void get_sub_requests (uint32_t sub_map[MAX_DEVICES + 1], process_t process);

/*
 * Should be called from all processes that want to know current state of the command map (i.e. device handler)
 * Blocks on the command bitmap semaphore for obvious reasons
 * Arguments:
 *    - uint32_t bitmap[MAX_DEVICES + 1]: pointer to array of 33 32-bit integers to copy the bitmap into. See the README for a
 *        description for how this bitmap works.
 * No return value.
 */
void get_cmd_map (uint32_t bitmap[MAX_DEVICES + 1]);

/*
 * Should be called from all processes that want to know device identifiers of all currently connected devices
 * Blocks on catalog semaphore for obvious reasons
 * Arguments:
 *    - dev_id_t dev_ids[MAX_DEVICES]: pointer to array of dev_id_t's to copy the information into
 * No return value.
 */
void get_device_identifiers (dev_id_t dev_ids[MAX_DEVICES]);

/*
 * Should be called from all processes that want to know which dev_ix's are valid
 * Blocks on catalog semaphore for obvious reasons
 * Arguments:
 *    - catalog: pointer to 32-bit integer into which the current catalog will be read into
 * No return value.
 */
void get_catalog (uint32_t *catalog);

/*
 * Reads the specified robot description field. Blocks on the robot description semaphore.
 * Arguments:
 *    - field: one of the robot_desc_val_t's defined above to read from
 * Returns one of the robot_desc_val_t's defined in runtime_util that is the current value of the requested field.
 */
robot_desc_val_t robot_desc_read (robot_desc_field_t field);

/*
 * Writes the specified value into the specified field. Blocks on the robot description semaphore.
 * Arguments:
 *    - robot_desc_field_t field: one of the robot_desc_val_t's defined above to write val to
 *    - robot_desc_val_t val: one of the robot_desc_vals defined in runtime_util.c to write to the specified field
 * No return value.
 */
void robot_desc_write (robot_desc_field_t field, robot_desc_val_t val);

/*
 * Reads current state of the gamepad to the provided pointers.
 * Blocks on both the gamepad semaphore and device description semaphore (to check if gamepad connected).
 * Arguments:
 *    - uint32_t pressed_buttons: pointer to 32-bit bitmap to which the current button bitmap state will be read into
 *    - float joystick_vals[4]: array of 4 floats to which the current joystick states will be read into
 * Returns 0 on success, -1 on failure (if gamepad is not connected)
 */
int gamepad_read (uint32_t *pressed_buttons, float joystick_vals[4]);

/*
 * This function writes the given state of the gamepad to shared memory.
 * Blocks on both the gamepad semaphore and device description semaphore (to check if gamepad connected).
 * Arguments:
 *    - uint32_t pressed_buttons: a 32-bit bitmap that corresponds to which buttons are currently pressed
            (only the first 17 bits used, since there are 17 buttons)
 *    - float joystick_vals[4]: array of 4 floats that contain the values to write to the joystick
 * Returns 0 on success, -1 on failure (if gamepad is not connected)
 */
int gamepad_write (uint32_t pressed_buttons, float joystick_vals[4]);

/**
* 	Write the given custom parameter to shared memory. 
*	Args:
*		- key is name of the parameter
*		- type is type of the parameter
*		- value is the value of the parameter, with the corresponding type filled with data
*	Returns:
*		0 if successful
*		-1 if the maximum number of custom parameters is reached so this parameter isn't written
*/
int log_data_write(char* key, param_type_t type, param_val_t value);

/**
*	Reads the custom log data from shared memory.
*	Args:
*		- num_params is a pointer to an int that will get filled with the number of custom parameters
*		- names is a 2D char array that will be filled with the parameter names, up to `num_params`. Assumes that every parameter name is less than 64 characters long
*		- types is an array and will be filled with the parameter types, up to `num_params`
*		- values is an array and will be filled with the parameter values, up to `num_params`
*/
void log_data_read(uint8_t* num_params, char names[UCHAR_MAX][64], param_type_t types[UCHAR_MAX], param_val_t values[UCHAR_MAX]);

#endif
