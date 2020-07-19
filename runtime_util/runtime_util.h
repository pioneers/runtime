#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <string.h>  // strcmp

// ***************************** DEFINED CONSTANTS ************************** //

#define MAX_DEVICES 32			//maximum number of connected devices
#define MAX_PARAMS 32			//maximum number of parameters supported

#define NUM_DEVICES 11 			// The number of functional devices
#define DEVICES_LENGTH 14		// The largest device type number + 1. Note: DEVICES_LENGTH != NUM_DEVICES because some are NULL (Ex: type 6, 8, 9)

#define NUM_DESC_FIELDS 5		//number of fields in the robot description

#define NUM_GAMEPAD_BUTTONS 17	//number of gamepad buttons

#define MAX_LOG_LEN 512

#define NUM_CHALLENGES 2
#define CHALLENGE_LEN 128
#define CHALLENGE_SOCKET "/tmp/challenge.sock"

//enumerate names of processes
typedef enum process {
	DEV_HANDLER, EXECUTOR, NET_HANDLER, SHM, TEST
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
	IDLE, AUTO, TELEOP, CHALLENGE,   //values for robot.run_mode
	CONNECTED, DISCONNECTED,         //values for robot.dawn, robot.shepherd, robot.gamepad
	LEFT, RIGHT                      //values for robot.startpos
} robot_desc_val_t;

//enumerated names for the fields in the robot description
typedef enum robot_descs {
	RUN_MODE, DAWN, SHEPHERD, GAMEPAD, START_POS
} robot_desc_field_t;

//enumerated names of device types
//TODO: Consider changing dev_id_t.type to use this uint8_t enum
/*
typedef enum dev_type {
	LIMIT_SWITCH = 0,
	LINE_FOLLOWER = 1,
	POTENTIOMETER = 2,
	ENCODER = 3,
	BATTERY_BUZZER = 4,
	TEAM_FLAG = 5,
	// GRIZZLY = 6,
	SERVO_CONTROL = 7,
	// LINEAR_ACTUATOR = 8,
	// COLOR_SENSOR = 9,
	YOGI_BEAR = 10,
	RFID = 11,
	POLAR_BEAR = 12,
	KOALA_BEAR = 13
	// DISTANCE_SENSOR = 16
	// METAL_DETECTOR = 17
} dev_type_t; */

//enumerated names for the data types device parameters can be
typedef enum param_type {
	INT, FLOAT, BOOL
} param_type_t;

// ******************************* CUSTOM STRUCTS ************************** //

//hold a single param value. One-to-one mapping to param_val_t enum
typedef union {
	int16_t p_i;	//data if int
	float p_f;		//data if float
	uint8_t p_b;	//data if bool
} param_val_t;

//holds the device identification information of a single device
typedef struct dev_id {
	uint16_t type;
	uint8_t year;
	uint64_t uid;
} dev_id_t;

// A struct defining the type and access level of a device parameter
typedef struct param_desc {
  char* name;			// Parameter name
  param_type_t type;	// Data type
  uint8_t read;			// Whether or not the param is readable
  uint8_t write;		// Whether or not the param is writable
} param_desc_t;

// A struct defining a kind of device (ex: LimitSwitch, KoalaBear)
typedef struct device {
  uint16_t type;					// The type of device
  char* name;						// Device name (ex: "LimitSwitch")
  uint8_t num_params; 				// Number of params the device has
  param_desc_t params[MAX_PARAMS];	// Description of each parameter
} device_t;


// *************************** DEVICE UTILITY FUNCTIONS ************************** //

/* Returns a pointer to the device given its DEVICE_TYPE */
device_t* get_device(uint16_t device_type);

/* Returns the device type given its device name DEV_NAME */
uint16_t device_name_to_type(char* dev_name);

/* Returns the device name given its type DEVICE_TYPE */
char* get_device_name(uint16_t device_type)

/* Return the description of the device type's parameter. */
param_desc_t* get_param_desc(uint16_t dev_type, char* param_name);

/* Get the param id of PARAM_NAME for the device DEV_TYPE */
int8_t get_param_idx(uint16_t dev_type, char* param_name);

/* Get array of strings, each representing the name of a button */
char** get_button_names();

/* Get array of strings, each representing the name of a joystick */
char** get_joystick_names();

#endif
