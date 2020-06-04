#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

// ***************************** DEFINED CONSTANTS ************************** //

//enumerate names of processes
typedef enum process { DEV_HANDLER, EXECUTOR, NET_HANDLER } process_t;

#define MAX_DEVICES 32 //maximum number of connected devices
#define MAX_PARAMS 32 //maximum number of parameters per connected device

// ******************************* CUSTOM STRUCTS ************************** //

//hold a single param value
typedef struct param {
	int p_i;       //data if int
	float p_f;     //data if float
	uint8_t p_b;   //data if bool
} param_val_t;

//holds the device identification information of a single device
typedef struct dev_id {
	uint16_t type;
	uint8_t year;
	uint64_t uid;
} dev_id_t;

// A struct defining the params of a device
// See devices.c for defined values
typedef struct param_desc {
  char* name;
  char* type;		// Either "int", "float", or "bool"
  uint8_t read;		// 8 bit value respresenting true or false
  uint8_t write;	// 8 bit value respresenting true or false
} param_desc_t;

// A struct defining a kind of device (ex: LimitSwitch, KoalaBear)
typedef struct device {
  uint16_t type;
  char* name; //Device name
  uint8_t num_params; //Number of parameters a device holds
  param_desc_t params[MAX_PARAMS]; // There are up to 32 possible parameters for a device
} device_t;

#endif