#ifndef SHM_WRAPPER_H
#define SHM_WRAPPER_H

#include <limits.h>     // for UCHAR_MAX
#include <semaphore.h>  // for semaphores
#include <stdbool.h>
#include <sys/mman.h>  // for posix shared memory

#include <logger.h>              // for logger
#include <runtime_util.h>        // for runtime constants

// names of various objects used in shm_wrapper; should not be used outside of shm_wrapper.c, shm_start.c, and shm_stop.c
#define DEV_SHM_NAME "/dev-shm"        // name of shared memory block across devices
#define CATALOG_MUTEX_NAME "/cat-sem"  // name of semaphore used as a mutex on the catalog
#define CMDMAP_MUTEX_NAME "/cmap-sem"  // name of semaphore used as a mutex on the command bitmap

#define INPUTS_SHM_NAME "/inputs-shm"    // name of shared memory block for inputs
#define INPUTS_MUTEX_NAME "/inputs-sem"  // name of semaphore used as mutex over inputs shm

#define ROBOT_DESC_SHM_NAME "/rd-shm"  // name of shared memory block for robot description
#define RD_MUTEX_NAME "/rd-sem"        // name of semaphore used as mutex over robot description shm

#define LOG_DATA_SHM "/log-data-shm"    // name of shared memory block for Robot.log data
#define LOG_DATA_MUTEX "/log-data-sem"  // name of semaphore used as mutex over Robot.log shm

#define SNAME_SIZE 32  // size of buffers that hold semaphore names, in bytes

// *********************************** SHM TYPEDEFS  ****************************************************** //

// enumerated names for the two associated blocks per device
typedef enum stream {
    DATA,
    COMMAND
} stream_t;

// shared memory block that holds device information, data, and commands has this structure
typedef struct {
    uint32_t catalog;                                // catalog of valid devices
    uint32_t cmd_map[MAX_DEVICES + 1];               // bitmap is 33 32-bit integers (changed devices and changed params of device commands from executor to dev_handler)
    param_val_t params[2][MAX_DEVICES][MAX_PARAMS];  // all the device parameter info, data and commands
    dev_id_t dev_ids[MAX_DEVICES];                   // all the device identification info
} dev_shm_t;

// two mutex semaphores for each device
typedef struct {
    sem_t* data_sem;     // semaphore on the data stream of a device
    sem_t* command_sem;  // semaphore on the command stream of a device
} dual_sem_t;


// struct describing an input
typedef struct {
    uint64_t buttons;           // bitmap for which buttons are pressed
    float joysticks[4];         // array to hold joystick positions, only for gamepad
    robot_desc_field_t source;  // which hardware device the command came from, either GAMEPAD or KEYBOARD
} input_t;

// shared memory for gamepad and keyboard inputs
typedef struct {
    input_t inputs[2];  // Index 0 is for GAMEPAD, index 1 is for KEYBOARD, like in the enum. can be modified in the future
} input_shm_t;

// shared memory for robot description
typedef struct {
    uint8_t fields[NUM_DESC_FIELDS];  // array to hold the robot state (each is a enum stored as a uint8_t)
} robot_desc_shm_t;


// shared memory for Robot.log data
typedef struct {
    uint8_t num_params;                     // number of quantities the student wants to log
    char names[UCHAR_MAX][LOG_KEY_LENGTH];  // keys (names) of quantities that student wants to log
    param_val_t params[UCHAR_MAX];          // values of quantities that student wants to log
    param_type_t types[UCHAR_MAX];          // types of the values that student wants to log
} log_data_shm_t;

// *********************************** SHM EXTERNAL VARIABLES  ******************************************** //

// DO NOT USE THESE UNDER NORMAL CIRCUMSTANCES
// THESE ARE ONLY USED TO SIMPLIFY CODE IN SHM_START AND SHM_STOP

extern dual_sem_t sems[MAX_DEVICES];  // array of semaphores, two for each possible device (one for data and one for commands)
extern dev_shm_t* dev_shm_ptr;        // points to memory-mapped shared memory block for device data and commands
extern sem_t* catalog_sem;            // semaphore used as a mutex on the catalog
extern sem_t* cmd_map_sem;            // semaphore used as a mutex on the command bitmap

extern input_shm_t* input_shm_ptr;    // points to memory-mapped shared memory block for user inputs
extern robot_desc_shm_t* rd_shm_ptr;  // points to memory-mapped shared memory block for robot description
extern sem_t* input_sem;              // semaphore used as a mutex on the inputs
extern sem_t* rd_sem;                 // semaphore used as a mutex on the robot description

extern log_data_shm_t* log_data_shm_ptr;  // points to shared memory block for log data specified by executor
extern sem_t* log_data_sem;               // semaphore used as a mutex on the log data

// ******************************************* WRAPPER FUNCTIONS ****************************************** //

// Returns true iff shared memory exists.
bool shm_exists();

/**
 * Function that generates a semaphore name for the data and command streams
 * Should only be externally called by shm_start and shm_stop programs
 * Arguments:
 *    stream: stream for which the semaphore is created (one of DATA or COMMAND)
 *    dev_ix: index of device whose STREAM is being created
 *    name: pointer to a buffer of size SNAME_SIZE into which the name of the semaphore will be put
 */
void generate_sem_name(stream_t stream, int dev_ix, char* name);

/**
 * Returns the index in the SHM block of the specified device if it exists (-1 if it doesn't)
 * Arguments:
 *    dev_uid: 64-bit unique ID of the device
 * Returns: device index in shared memory of the specified device, -1 if specified device is not in shared memory
 */
int get_dev_ix_from_uid(uint64_t dev_uid);

/**
 * Call this function from every process that wants to use the shared memory wrapper
 * No return value (will exit on fatal errors).
 * Will configure process to close all shared memory and semaphores on process exit.
 */
void shm_init();

/**
 * Should only be called from device handler
 * Selects the next available device index and assigns the newly connected device to that location.
 * Turns on the associated bit in the catalog.
 * Arguments:
 *    dev_id: device identification info for the device being connected (type, year, uid)
 *    dev_ix: the index that the device was assigned will be put here
 * Returns device index of connected device in dev_ix on success; sets *dev_ix = -1 on failure
 */
void device_connect(dev_id_t* dev_id, int* dev_ix);

/**
 * Should only be called from device handler
 * Disconnects a device with a given index by turning off the associated bit in the catalog.
 * Arguments
 *    dev_ix: index of device in catalog to be disconnected
 */
void device_disconnect(int dev_ix);

/**
 * Should be called from every process wanting to read the device data
 * Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
 * Arguments:
 *    dev_ix: device index of the device whose data is being requested
 *    process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
 *    stream: the requested block to read from, one of DATA, COMMAND
 *    params_to_read: bitmap representing which params to be read  (nonexistent params should have corresponding bits set to 0)
 *    params: pointer to array of param_val_t's that is at least as long as highest requested param number
 *        device data will be read into the corresponding param_val_t's
 * Returns:
 *    0 on success
 *    -1 on failure (specified device is not connected in shm)
 */
int device_read(int dev_ix, process_t process, stream_t stream, uint32_t params_to_read, param_val_t* params);

/**
 * This function is the exact same as the above function, but instead uses the 64-bit device UID to identify
 * the device that should be read, rather than the device index.
 */
int device_read_uid(uint64_t dev_uid, process_t process, stream_t stream, uint32_t params_to_read, param_val_t* params);

/**
 * Should be called from every process wanting to write to the device data
 * Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
 * Grabs either one or two semaphores depending on calling process and stream requested.
 * Arguments:
 *    dev_ix: device index of the device whose data is being written
 *    process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
 *    stream: the requested block to write to, one of DATA, COMMAND
 *    params_to_read: bitmap representing which params to be written (nonexistent params should have corresponding bits set to 0)
 *    params: pointer to array of param_val_t's that is at least as long as highest requested param number
 *        device data will be written into the corresponding param_val_t's
 * Returns:
 *    0 on success
 *    -1 on failure (specified device is not connected in shm)
 */
int device_write(int dev_ix, process_t process, stream_t stream, uint32_t params_to_write, param_val_t* params);

/**
 * This function is the exact same as the above function, but instead uses the 64-bit device UID to identify
 * the device that should be written, rather than the device index.
 */
int device_write_uid(uint64_t dev_uid, process_t process, stream_t stream, uint32_t params_to_write, param_val_t* params);

/**
 * Should be called from all processes that want to know current state of the command map (i.e. device handler)
 * Blocks on the command bitmap semaphore for obvious reasons
 * Arguments:
 *    bitmap[MAX_DEVICES + 1]: pointer to array of 33 32-bit integers to copy the bitmap into. See the README for a
 *        description for how this bitmap works.
 */
void get_cmd_map(uint32_t bitmap[MAX_DEVICES + 1]);

/**
 * Should be called from all processes that want to know device identifiers of all currently connected devices
 * Blocks on catalog semaphore for obvious reasons
 * Arguments:
 *    dev_id_t dev_ids[MAX_DEVICES]: pointer to array of dev_id_t's to copy the information into
 */
void get_device_identifiers(dev_id_t dev_ids[MAX_DEVICES]);

/**
 * Should be called from all processes that want to know which dev_ix's are valid
 * Blocks on catalog semaphore for obvious reasons
 * Arguments:
 *    catalog: pointer to 32-bit integer into which the current catalog will be read into
 */
void get_catalog(uint32_t* catalog);

/**
 * Reads the specified robot description field. Blocks on the robot description semaphore.
 * Arguments:
 *    field: one of the robot_desc_val_t's defined above to read from
 * Returns one of the robot_desc_val_t's defined in runtime_util that is the current value of the requested field.
 */
robot_desc_val_t robot_desc_read(robot_desc_field_t field);

/**
 * Writes the specified value into the specified field. Blocks on the robot description semaphore.
 * Arguments:
 *    field: one of the robot_desc_val_t's defined above to write val to
 *    val: one of the robot_desc_vals defined in runtime_util.c to write to the specified field
 */
void robot_desc_write(robot_desc_field_t field, robot_desc_val_t val);

/**
 * Reads current state of the gamepad to the provided pointers.
 * Blocks on both the gamepad semaphore and device description semaphore (to check if gamepad connected).
 * Arguments:
 *    pressed_buttons: pointer to 64-bit bitmap to which the current button bitmap state will be read into
 *    joystick_vals[4]: array of 4 floats to which the current joystick states will be read into
 *    source: which input source to read from, either GAMEPAD or KEYBOARD
 * Returns:
 *    0 on success
 *    -1 if input is not connected
 */
int input_read(uint64_t* pressed_buttons, float joystick_vals[4], robot_desc_field_t source);

/**
 * This function writes the given state of the gamepad to shared memory.
 * Blocks on both the gamepad semaphore and device description semaphore (to check if gamepad connected).
 * Arguments:
 *    pressed_buttons: a 64-bit bitmap that corresponds to which buttons are currently pressed.
 *                     only some of the bits are used, depending on the input source
 *    joystick_vals[4]: array of 4 floats that contain the values to write to the joystick
 *    source: which input source to write to, either GAMEPAD or KEYBOARD
 * Returns:
 *    0 on success
 *    -1 if gamepad is not connected
 */
int input_write(uint64_t pressed_buttons, float joystick_vals[4], robot_desc_field_t source);

/**
 * Write the given custom parameter to shared memory.
 * Arguments:
 *    key: name of the parameter. Must be at most LOG_KEY_LENGTH characters, including the null terminator
 *    type: type of the parameter
 *    value: value of the parameter, with the corresponding type filled with data
 * Returns:
 *    0 on success
 *   -1 when maximum number of custom parameters is reached
 *   -2 when the given key is longer than LOG_KEY_LENGTH characters, including the null terminator
 */
int log_data_write(char* key, param_type_t type, param_val_t value);

/**
 * Reads the custom log data from shared memory.
 * Arguments:
 *    num_params: pointer to an int that will get filled with the number of custom parameters
 *    names: 2D char array that will be filled with the parameter names, up to `num_params`. Ever name is at most LOG_KEY_LENGTH characters long
 *    types: array that will be filled with the parameter types, up to `num_params`
 *    values: array that will be filled with the parameter values, up to `num_params`
 */
void log_data_read(uint8_t* num_params, char names[UCHAR_MAX][LOG_KEY_LENGTH], param_type_t types[UCHAR_MAX], param_val_t values[UCHAR_MAX]);

#endif
