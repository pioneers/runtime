/* Defines devices.json in C to avoid reading from JSON file everytimeget
A Device is defined by a 16-bit type, an 8-bit year, and a 64-bit uid*
(There's a 1-to-1 mapping between types and device names)
See hibikeDevices.json for the numbers. */
#ifndef DEVICES_H
#define DEVICES_H

#include <string.h>                        //for standard string functions
#include "../runtime_util/runtime_util.h"

/* The number of functional devices */
#define NUM_DEVICES 11

/*
 * The largest device type number + 1
 * DEVICES_LENGTH != NUM_DEVICES because some are NULL (Ex: type 6, 8, 9)
*/
#define DEVICES_LENGTH 14

/* Returns a pointer to the device given its DEVICE_TYPE */
device_t* get_device(uint16_t device_type);
/* Returns the device type given its device name DEV_NAME */
uint16_t device_name_to_type(char* dev_name);
/* Returns the name of a device given the DEV_TYPE. */
char* device_type_to_name(uint16_t dev_type);
/* Fills PARAM_NAMES with all the names of the parameters for the given device type */
void all_params_for_device_type(uint16_t dev_type, char* param_names[]);
/* Returns 1 if PARAM of device type is readable. Otherwise, 0. */
uint8_t readable(uint16_t dev_type, char* param_name);
/* Returns 1 if PARAM of device type is writable. Otherwise, 0. */
uint8_t writeable(uint16_t dev_type, char* param_name);
/* Return the data type of the device type's parameter ("int", "float", or "bool"). */
char* get_param_type(uint16_t dev_type, char* param_name);
/* Get the param id of PARAM_NAME for the device DEV_TYPE */
uint8_t get_param_id(uint16_t dev_type, char* param_name);

#endif
