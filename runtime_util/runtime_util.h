#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>    // perror()
#include <stdint.h>
#include <string.h>   // strcmp
#include <errno.h>
#include <unistd.h>   // read() and write()
#include <sys/time.h> // Used in millis()

// ***************************** DEFINED CONSTANTS ************************** //

#define MAX_DEVICES 32          // Maximum number of connected devices
#define MAX_PARAMS 32           // Maximum number of parameters supported

#define DEVICES_LENGTH 64      // The largest device type number + 1.

#define NUM_DESC_FIELDS 5       // Number of fields in the robot description

#define NUM_GAMEPAD_BUTTONS 17  // Number of gamepad buttons

#define MAX_LOG_LEN 512

#define CHALLENGE_LEN 128
#define CHALLENGE_SOCKET "/tmp/challenge.sock"

//enumerate names of processes
typedef enum process {
	DEV_HANDLER, EXECUTOR, NET_HANDLER, SHM, TEST
} process_t;

//enumerated names for the buttons on the gamepad
typedef enum gp_buttons {
	BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y, L_BUMPER, R_BUMPER, L_TRIGGER, R_TRIGGER,
	BUTTON_BACK, BUTTON_START, L_STICK, R_STICK, DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT, BUTTON_XBOX
} gp_button_t;

//enumerated names for the joystick params of the gamepad
typedef enum gp_joysticks {
	JOYSTICK_LEFT_X, JOYSTICK_LEFT_Y, JOYSTICK_RIGHT_X, JOYSTICK_RIGHT_Y
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

//enumerated names for the data types device parameters can be
typedef enum param_type {
	INT, FLOAT, BOOL
} param_type_t;

// ************************** CUSTOM STRUCTS ************************** //

//hold a single param value. One-to-one mapping to param_val_t enum
typedef union {
	int32_t p_i; //data if int
	float p_f;   //data if float
	uint8_t p_b; //data if bool
} param_val_t;

//holds the device identification information of a single device
typedef struct dev_id {
	uint8_t type;
	uint8_t year;
	uint64_t uid;
} dev_id_t;

// A struct defining the type and access level of a device parameter
typedef struct param_desc {
	char* name;        // Parameter name
	param_type_t type; // Data type
	uint8_t read;      // Whether or not the param is readable
	uint8_t write;     // Whether or not the param is writable
} param_desc_t;

// A struct defining a kind of device (ex: LimitSwitch, KoalaBear)
typedef struct device {
	uint8_t type;                    // The type of device
	char* name;                      // Device name (ex: "LimitSwitch")
	uint8_t num_params;              // Number of params the device has
	param_desc_t params[MAX_PARAMS]; // Description of each parameter
} device_t;

// *************************** DEVICE UTILITY FUNCTIONS ************************** //

/* Returns a pointer to the device given its DEVICE_TYPE or NULL if the device doesn't exist. */
device_t* get_device(uint8_t dev_type);

/* Returns the device type given its device name DEV_NAME */
uint8_t device_name_to_type(char* dev_name);

/* Returns the device name given its type DEVICE_TYPE or NULL if the device doesn't exist. */
char* get_device_name(uint8_t dev_type);

/* Return the description of the device type's parameter. */
param_desc_t* get_param_desc(uint8_t dev_type, char* param_name);

/* Get the param id of PARAM_NAME for the device DEV_TYPE */
int8_t get_param_idx(uint8_t dev_type, char* param_name);

/* Get array of strings, each representing the name of a button */
char** get_button_names();

/* Get array of strings, each representing the name of a joystick */
char** get_joystick_names();

// *************************** READ/WRITE TO FILE DESCRIPTOR ************************** //

/* Returns the number of milliseconds since the Unix Epoch */
uint64_t millis();

/*
 * Read n bytes from fd into buf; return number of bytes read into buf (deals with interrupts and unbuffered reads)
 * Arguments:
 *    - int fd: file descriptor to read from
 *    - void *buf: pointer to location to copy read bytes into
 *    - size_t n: number of bytes to read
 * Return:
 *    - > 0: number of bytes read into buf
 *    - 0: read EOF on fd
 *    - -1: read errored out
 */
int readn (int fd, void *buf, uint16_t n);

/*
 * Read n bytes from buf to fd; return number of bytes written to buf (deals with interrupts and unbuffered writes)
 * Arguments:
 *    - int fd: file descriptor to write to
 *    - void *buf: pointer to location to read from
 *    - size_t n: number of bytes to write
 * Return:
 *    - >= 0: number of bytes written into buf
 *    - -1: write errored out
 */
int writen (int fd, void *buf, uint16_t n);

#endif
