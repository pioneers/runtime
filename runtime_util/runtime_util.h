#ifndef UTIL_H
#define UTIL_H

// ************************* COMMON STANDARD HEADERS ************************ //

#include <stdio.h>      // for printf, perror, fprintf, fopen, etc.
#include <stdlib.h>     // for malloc, free
#include <stdint.h>     // for uint32_t, int16_t, uint8_t, etc.
#include <string.h>     // for strcmp, strcpy, strlen, memset, etc.
#include <errno.h>      // for various error numbers (EINTR, EAGAIN, EPIPE, etc.) and errno
#include <pthread.h>    // for thread-functions: pthread_create, pthread_cancel, pthread_join, etc.
#include <unistd.h>     // for F_OK, R_OK, SEEK_SET, SEEK_END, access, ftruncate, read, write, etc.
#include <signal.h>     // for setting signal function (setting signal handler)
#include <sys/time.h>   // for time-related structures and time functions
#include <fcntl.h>      // for file opening constants (O_CREAT, O_RDONLY, O_RDWR, etc.)
#include <sys/stat.h>   // for various system-related types and functions (sem_t, mkfifo)
#include <sys/socket.h> // for bind, listen, accept, socket
#include <sys/un.h>     // for struct sockaddr_un

// ***************************** DEFINED CONSTANTS ************************** //

#define MAX_DEVICES 32          // Maximum number of connected devices
#define MAX_PARAMS 32           // Maximum number of parameters supported

#define DEVICES_LENGTH 64       // The largest device type number + 1.

#define NUM_DESC_FIELDS 5       // Number of fields in the robot description

#define NUM_GAMEPAD_BUTTONS 17  // Number of gamepad buttons

#define MAX_LOG_LEN 512         // The maximum number of characters in a log message

#define CHALLENGE_LEN 128       // The maximum input/output string length for a challenge
#define CHALLENGE_SOCKET "/tmp/challenge.sock"

// enumerated names of processes
typedef enum process {
    DEV_HANDLER, EXECUTOR, NET_HANDLER, SHM, TEST
} process_t;

// enumerated names for the buttons on the gamepad
typedef enum gp_buttons {
    A_BUTTON, B_BUTTON, X_BUTTON, Y_BUTTON, L_BUMPER, R_BUMPER, L_TRIGGER, R_TRIGGER,
    BACK_BUTTON, START_BUTTON, L_STICK, R_STICK, UP_DPAD, DOWN_DPAD, LEFT_DPAD, RIGHT_DPAD, XBOX_BUTTON
} gp_button_t;

// enumerated names for the joystick params of the gamepad
typedef enum gp_joysticks {
    X_LEFT_JOYSTICK, Y_LEFT_JOYSTICK, X_RIGHT_JOYSTICK, Y_RIGHT_JOYSTICK
} gp_joystick_t;

// enumerated names for the different values the robot description fields can take on
typedef enum robot_desc_vals {
    IDLE, AUTO, TELEOP, CHALLENGE,   // values for robot.run_mode
    CONNECTED, DISCONNECTED,         // values for robot.dawn, robot.shepherd, robot.gamepad
    LEFT, RIGHT                      // values for robot.startpos
} robot_desc_val_t;

// enumerated names for the fields in the robot description
typedef enum robot_descs {
    RUN_MODE, DAWN, SHEPHERD, GAMEPAD, START_POS
} robot_desc_field_t;

// enumerated names for the data types device parameters can be
typedef enum param_type {
    INT, FLOAT, BOOL
} param_type_t;

// ***************************** CUSTOM STRUCTS ***************************** //

// hold a single param value. One-to-one mapping to param_val_t enum
typedef union {
    int32_t p_i; // data if int
    float p_f;   // data if float
    uint8_t p_b; // data if bool
} param_val_t;

// holds the device identification information of a single device
typedef struct dev_id {
    uint8_t type;   // The device type (ex: 0 for DummyDevice)
    uint8_t year;   // The device year
    uint64_t uid;   // The unique id for a specific device assigned when flashed
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

// ************************ DEVICE UTILITY FUNCTIONS ************************ //

/**
 * Returns a pointer to a device given its type.
 * Arguments:
 *    dev_type: The device type
 * Returns:
 *    A pointer to the device, or
 *    NULL if the device doesn't exist.
 */
device_t* get_device(uint8_t dev_type);

/**
 * Returns the device type of a device given its name.
 * Arguments:
 *    dev_name: The name of the device
 * Returns:
 *    The device type, or
 *    -1 if the device doesn't exist.
 */
uint8_t device_name_to_type(char* dev_name);

/**
 * Returns the name of the device given its type.
 * Arguments:
 *    dev_type: The device type
 * Returns:
 *    The device name, or
 *    NULL if the device doesn't exist.
 */
char* get_device_name(uint8_t dev_type);

/**
 * Returns a parameter descriptor.
 * Arguments:
 *    dev_type: The device type with the parameter of interest
 *    param_name: The name of the parameter
 * Returns:
 *    The parameter descriptor, or
 *    NULL if the device doesn't exist or the parameter doesn't exist
 */
param_desc_t* get_param_desc(uint8_t dev_type, char* param_name);

/**
 * Return the index of a parameter.
 * Arguments:
 *    dev_type: The device type
 *    param_name: The name of the parameter
 * Returns:
 *    The parameter's index for the device, or
 *    -1 if the device doesn't exist or the parameter doesn't exist.
 */
int8_t get_param_idx(uint8_t dev_type, char* param_name);

/**
 * Returns an array of button names.
 */
char** get_button_names();

/**
 * Returns an array of joystick names.
 */
char** get_joystick_names();

// ********************************** TIME ********************************** //

/**
 * Returns the number of milliseconds since the Unix Epoch.
 */
uint64_t millis();

// ********************* READ/WRITE TO FILE DESCRIPTOR ********************** //

/**
 * Read n bytes from fd into buf; return number of bytes read into buf (deals with interrupts and unbuffered reads)
 * Arguments:
 *    fd: file descriptor to read from
 *    buf: pointer to location to copy read bytes into
 *    n: number of bytes to read
 * Returns:
 *    > 0: number of bytes read into buf
 *    0: read EOF on fd
 *    -1: read errored out
 */
int readn(int fd, void *buf, uint16_t n);

/**
 * Read n bytes from buf to fd; return number of bytes written to buf (deals with interrupts and unbuffered writes)
 * Arguments:
 *    fd: file descriptor to write to
 *    buf: pointer to location to read from
 *    n: number of bytes to write
 * Returns:
 *    >= 0: number of bytes written into buf
 *    -1: write errored out
 */
int writen(int fd, void *buf, uint16_t n);

#endif
