/* Defines devices.json in C to avoid reading from JSON file everytime
A Device is defined by a 16-bit type, an 8-bit year, and a 64-bit uid*
(There's a 1-to-1 mapping between types and device names)
See hibikeDevices.json for the numbers. */
#ifndef DEVICES_H
#define DEVICES_H

#include <string.h>                        //for standard string functions
#include "../runtime_util.h"

/* The number of functional devices */
#define NUM_DEVICES 11
/* The largest device type number + 1
 * DEVICES_LENGTH != NUM_DEVICES because some are NULL (Ex: type 6, 8, 9)
*/
#define DEVICES_LENGTH 14

device_t* DEVICES[DEVICES_LENGTH];

/* Puts all devices into one array. */
void get_devices(device_t* devices[]);
/* Converts a device name to the 16-bit type. */
uint16_t device_name_to_type(char* dev_name);
/* Converts a device type to its name */
char* device_type_to_name(uint16_t dev_type);
/* Returns an array of param name strings given a device type */
void all_params_for_device_type(uint16_t dev_type, char* param_names[]);
/* Check if PARAM of device type is readable */
int readable(uint16_t dev_type, char* param_name);
/* Check if PARAM of device type is writable */
int writeable(uint16_t dev_type, char* param_name);
/* Return the data type of the device type's parameter */
char* get_param_type(uint16_t dev_type, char* param_name);
/* Get the param id in array from param name */
uint8_t get_param_id(uint16_t dev_type, char* param_name);


#endif