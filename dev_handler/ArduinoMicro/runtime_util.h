#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <string.h>  // strcmp
#include <stdlib.h>
#include <sys/types.h>
// ***************************** DEFINED CONSTANTS ************************** //

#define MAX_DEVICES 32 //maximum number of connected devices
#define MAX_PARAMS 32 //maximum number of parameters per connected device

#define NUM_DESC_FIELDS 5                   //number of fields in the robot description
#define NUM_GAMEPAD_BUTTONS 17              //number of gamepad buttons

#define NUM_DEVICES 11 		// The number of functional devices
#define DEVICES_LENGTH 14 	// The largest device type number + 1. Note: DEVICES_LENGTH != NUM_DEVICES because some are NULL (Ex: type 6, 8, 9)

//enumerate names of processes
typedef enum process {
	DEV_HANDLER, EXECUTOR, NET_HANDLER, STUDENTAPI
} process_t;

//enumerated names for the buttons on the gamepad
typedef enum gp_buttons {
	A_BUTTON, B_BUTTON, X_BUTTON, Y_BUTTON, L_BUMPER, R_BUMPER, L_TRIGGER, R_TRIGGER,
	BACK_BUTTON, START_BUTTON, L_STICK, R_STICK, UP_DPAD, DOWN_DPAD, LEFT_DPAD, RIGHT_DPAD, XBOX_BUTTON
} gp_button_t;

//enumerated names for the joystick params of the gamepad
typedef enum gp_joysticks {
	X_LEFT_JOYSTICK, Y_LEFT_JOYSTICK, X_RIGHT_JOYSTICK, Y_RIGHT_JOYSTICK
} gp_joystick_t;

//enumerated names for the different values the robot description fields can take on
typedef enum robot_desc_vals {
	IDLE, AUTO, TELEOP,         //values for robot.run_mode
	CONNECTED, DISCONNECTED,    //values for robot.dawn, robot.shepherd, robot.gamepad
	BLUE, GOLD                  //values for robot.team
} robot_desc_val_t;

//enumerated names for the fields in the robot description
typedef enum robot_descs {
	RUN_MODE, DAWN, SHEPHERD, GAMEPAD, TEAM
} robot_desc_field_t;

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

// *************************** DEVICE UTILITY FUNCTIONS ************************** //

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