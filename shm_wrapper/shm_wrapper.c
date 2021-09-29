#include "shm_wrapper.h"

// *********************************** WRAPPER-SPECIFIC GLOBAL VARS **************************************** //

dual_sem_t sems[MAX_DEVICES];  // array of semaphores, two for each possible device (one for data and one for commands)
dev_shm_t* dev_shm_ptr;        // points to memory-mapped shared memory block for device data and commands
sem_t* catalog_sem;            // semaphore used as a mutex on the catalog
sem_t* cmd_map_sem;            // semaphore used as a mutex on the command bitmap

input_shm_t* input_shm_ptr;    // points to memory-mapped shared memory block for user inputs
robot_desc_shm_t* rd_shm_ptr;  // points to memory-mapped shared memory block for robot description
sem_t* input_sem;              // semaphore used as a mutex on the inputs
sem_t* rd_sem;                 // semaphore used as a mutex on the robot description

log_data_shm_t* log_data_shm_ptr;  // points to shared memory block for log data specified by executor
sem_t* log_data_sem;               // semaphore used as a mutex on the log data

// ****************************************** EMERGENCY CONTROL ***************************************** //

/**
 * Send a command to stop all moving parts on the robot. State of the game is unaffected.
 * 
 * Depending on the state of Runtime, it may be wise to emergency stop the robot.
 * Note that this does not block further commands to move the robot; that should be implemented
 * in the student API. (This sends only ONE stop command that can be overwritten if not careful.)
 * 
 * The implementation of this function may change annually based on the relevant
 * devices and parameters.
 * Devices affected:
 * - KoalaBear: velocity_a, velocity_b
 */
static void stop_robot() {
    uint8_t koalabear = device_name_to_type("KoalaBear");
    uint32_t params_to_write = (1 << get_param_idx(koalabear, "velocity_a")) | (1 << get_param_idx(koalabear, "velocity_b"));
    // Get currently connected devices
    uint32_t catalog = 0;
    get_catalog(&catalog);
    dev_id_t dev_ids[MAX_DEVICES] = {0};
    get_device_identifiers(dev_ids);
    // Zero out parameters that move the robot
    param_val_t params_zero[MAX_PARAMS] = {0};
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (catalog & (1 << i)) {  // Device i is connected
            if (dev_ids[i].type == koalabear) {
                device_write(i, SHM, COMMAND, params_to_write, params_zero);
            }
        }
    }
}

// ****************************************** SEMAPHORE UTILITIES ***************************************** //

/**
 * Custom wrapper function for sem_wait. Prints out descriptive logging message on failure
 * Arguments:
 *    sem: pointer to a semaphore to wait on
 *    sem_desc: string that describes the semaphore being waited on, displayed with error message
 */
static void my_sem_wait(sem_t* sem, char* sem_desc) {
    if (sem_wait(sem) == -1) {
        log_printf(ERROR, "sem_wait: %s. %s", sem_desc, strerror(errno));
    }
}

/**
 * Custom wrapper function for sem_post. Prints out descriptive logging message on failure
 * Arguments:
 *    sem: pointer to a semaphore to post
 *    sem_desc: string that describes the semaphore being posted, displayed with error message
 */
static void my_sem_post(sem_t* sem, char* sem_desc) {
    if (sem_post(sem) == -1) {
        log_printf(ERROR, "sem_post: %s. %s", sem_desc, strerror(errno));
    }
}

/**
 * Custom wrapper function for sem_open. Prints out descriptive logging message on failure
 * exits (not being able to open a semaphore is fatal to Runtime).
 * Arguments:
 *    sem_name: name of a semaphore to be opened
 *    sem_desc: string that describes the semaphore being opened, displayed with error message
 * Returns a pointer to the semaphore that was opened.
 */
static sem_t* my_sem_open(char* sem_name, char* sem_desc) {
    sem_t* ret;
    if ((ret = sem_open(sem_name, 0, 0, 0)) == SEM_FAILED) {
        log_printf(FATAL, "sem_open: %s. %s", sem_desc, strerror(errno));
        exit(1);
    }
    return ret;
}

/**
 * Custom wrapper function for sem_close. Prints out descriptive logging message on failure
 * Arguments:
 *    sem: pointer to a semaphore to close
 *    sem_desc: string that describes the semaphore being closed, displayed with error message
 */
static void my_sem_close(sem_t* sem, char* sem_desc) {
    if (sem_close(sem) == -1) {
        log_printf(ERROR, "sem_close: %s. %s", sem_desc, strerror(errno));
    }
}

// ******************************************** HELPER FUNCTIONS ****************************************** //

/**
 * Function that does the actual reading into shared memory for device_read and device_read_uid
 * Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
 * Arguments:
 *    dev_ix: device index of the device whose data is being requested
 *    process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
 *    stream: the requested block to read from, one of DATA, COMMAND
 *    params_to_read: bitmap representing which params to be read  (nonexistent params should have corresponding bits set to 0)
 *    params: pointer to array of param_val_t's that is at least as long as highest requested param number
 *        device data will be read into the corresponding param_val_t's
 */
static void device_read_helper(int dev_ix, process_t process, stream_t stream, uint32_t params_to_read, param_val_t* params) {
    // grab semaphore for the appropriate stream and device
    if (stream == DATA) {
        my_sem_wait(sems[dev_ix].data_sem, "data sem @device_read");
    } else {
        my_sem_wait(sems[dev_ix].command_sem, "command sem @device_read");
    }

    // read all requested params
    for (int i = 0; i < MAX_PARAMS; i++) {
        if (params_to_read & (1 << i)) {
            params[i] = dev_shm_ptr->params[stream][dev_ix][i];
        }
    }

    // if the device handler has processed the command, then turn off the change
    // if stream = downstream and process = dev_handler then also update params bitmap
    if (process == DEV_HANDLER && stream == COMMAND) {
        // wait on cmd_map_sem
        my_sem_wait(cmd_map_sem, "cmd_map_sem @device_read");

        dev_shm_ptr->cmd_map[0] &= (~(1 << dev_ix));            // turn off changed device bit in cmd_map[0]
        dev_shm_ptr->cmd_map[dev_ix + 1] &= (~params_to_read);  // turn off bits for params that were changed and then read in cmd_map[dev_ix + 1]

        // release cmd_map_sem
        my_sem_post(cmd_map_sem, "cmd_map_sem @device_read");
    }

    // release semaphore for appropriate stream and device
    if (stream == DATA) {
        my_sem_post(sems[dev_ix].data_sem, "data sem @device_read");
    } else {
        my_sem_post(sems[dev_ix].command_sem, "command sem @device_read");
    }
}

/**
 * Function that does the actual writing into shared memory for device_write and device_write_uid
 * Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
 * Grabs either one or two semaphores depending on calling process and stream requested.
 * Arguments:
 *    dev_ix: device index of the device whose data is being written
 *    process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
 *    stream: the requested block to write to, one of DATA, COMMAND
 *    params_to_read: bitmap representing which params to be written (nonexistent params should have corresponding bits set to 0)
 *    params: pointer to array of param_val_t's that is at least as long as highest requested param number
 *        device data will be written into the corresponding param_val_t's
 */
static void device_write_helper(int dev_ix, process_t process, stream_t stream, uint32_t params_to_write, param_val_t* params) {
    // grab semaphore for the appropriate stream and device
    if (stream == DATA) {
        my_sem_wait(sems[dev_ix].data_sem, "data sem @device_write");
    } else {
        my_sem_wait(sems[dev_ix].command_sem, "command sem @device_write");
    }

    // write all requested params
    for (int i = 0; i < MAX_PARAMS; i++) {
        if (params_to_write & (1 << i)) {
            dev_shm_ptr->params[stream][dev_ix][i] = params[i];
        }
    }

    // If writing a command, update the command map to indicate which param should be changed
    if (stream == COMMAND) {
        // wait on cmd_map_sem
        my_sem_wait(cmd_map_sem, "cmd_map_sem @device_write");

        dev_shm_ptr->cmd_map[0] |= (1 << dev_ix);             // turn on changed device bit in cmd_map[0]
        dev_shm_ptr->cmd_map[dev_ix + 1] |= params_to_write;  // turn on bits for params that were written in cmd_map[dev_ix + 1]

        // release cmd_map_sem
        my_sem_post(cmd_map_sem, "cmd_map_sem @device_write");
    }

    // release semaphore for appropriate stream and device
    if (stream == DATA) {
        my_sem_post(sems[dev_ix].data_sem, "data sem @device_write");
    } else {
        my_sem_post(sems[dev_ix].command_sem, "command sem @device_write");
    }
}

/**
 * This function will be called when process that called shm_init() exits
 * Closes all semaphores; unmaps all shared memory (but does not unlink anything)
 */
static void shm_close() {
    // close all the semaphores
    for (int i = 0; i < MAX_DEVICES; i++) {
        my_sem_close(sems[i].data_sem, "data sem");
        my_sem_close(sems[i].command_sem, "command sem");
    }
    my_sem_close(catalog_sem, "catalog sem");
    my_sem_close(cmd_map_sem, "cmd map sem");
    my_sem_close(input_sem, "inputs_mutex");
    my_sem_close(rd_sem, "robot_desc_mutex");
    my_sem_close(log_data_sem, "log data mutex");

    // unmap all shared memory blocks
    if (munmap(dev_shm_ptr, sizeof(dev_shm_t)) == -1) {
        log_printf(ERROR, "munmap: dev_shm. %s", strerror(errno));
    }
    if (munmap(input_shm_ptr, sizeof(input_shm_t)) == -1) {
        log_printf(ERROR, "munmap: input_shm. %s", strerror(errno));
    }
    if (munmap(rd_shm_ptr, sizeof(robot_desc_shm_t)) == -1) {
        log_printf(ERROR, "munmap: robot_desc_shm. %s", strerror(errno));
    }
    if (munmap(log_data_shm_ptr, sizeof(log_data_shm_t)) == -1) {
        log_printf(ERROR, "munmap: log_data_shm_ptr. %s", strerror(errno));
    }
}

// ************************************ PUBLIC WRAPPER FUNCTIONS ****************************************** //

bool shm_exists() {
    // Assumes all shared memory blocks are created together and destroyed together
    return (access("/dev/shm/dev-shm", F_OK) == 0);
}

void generate_sem_name(stream_t stream, int dev_ix, char* name) {
    if (stream == DATA) {
        sprintf(name, "/data_sem_%d", dev_ix);
    } else if (stream == COMMAND) {
        sprintf(name, "/command_sem_%d", dev_ix);
    }
}

int get_dev_ix_from_uid(uint64_t dev_uid) {
    int dev_ix = -1;

    for (int i = 0; i < MAX_DEVICES; i++) {
        if ((dev_shm_ptr->catalog & (1 << i)) && (dev_shm_ptr->dev_ids[i].uid == dev_uid)) {
            dev_ix = i;
            break;
        }
    }
    return dev_ix;
}

void shm_init() {
    // Verify that shared memory exists
    if (!shm_exists()) {
        log_printf(ERROR, "shm_init: SHM does not exist!");
        exit(1);
    }

    int fd_shm;              // file descriptor of the memory-mapped shared memory
    char sname[SNAME_SIZE];  // for holding semaphore names

    // open all the semaphores
    catalog_sem = my_sem_open(CATALOG_MUTEX_NAME, "catalog mutex");
    cmd_map_sem = my_sem_open(CMDMAP_MUTEX_NAME, "cmd map mutex");
    input_sem = my_sem_open(INPUTS_MUTEX_NAME, "inputs mutex");
    rd_sem = my_sem_open(RD_MUTEX_NAME, "robot desc mutex");
    log_data_sem = my_sem_open(LOG_DATA_MUTEX, "log data mutex");
    for (int i = 0; i < MAX_DEVICES; i++) {
        generate_sem_name(DATA, i, sname);  // get the data name
        if ((sems[i].data_sem = sem_open((const char*) sname, 0, 0, 0)) == SEM_FAILED) {
            log_printf(ERROR, "sem_open: data sem for dev_ix %d: %s", i, strerror(errno));
        }
        generate_sem_name(COMMAND, i, sname);  // get the command name
        if ((sems[i].command_sem = sem_open((const char*) sname, 0, 0, 0)) == SEM_FAILED) {
            log_printf(ERROR, "sem_open: command sem for dev_ix %d: %s", i, strerror(errno));
        }
    }

    // open dev shm block and map to client process virtual memory
    if ((fd_shm = shm_open(DEV_SHM_NAME, O_RDWR, 0)) == -1) {  // no O_CREAT
        log_printf(FATAL, "shm_open dev_shm: %s", strerror(errno));
        exit(1);
    }
    if ((dev_shm_ptr = mmap(NULL, sizeof(dev_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
        log_printf(FATAL, "mmap dev_shm: %s", strerror(errno));
        exit(1);
    }
    if (close(fd_shm) == -1) {
        log_printf(ERROR, "close dev_shm: %s", strerror(errno));
    }

    // open gamepad shm block and map to client process virtual memory
    if ((fd_shm = shm_open(INPUTS_SHM_NAME, O_RDWR, 0)) == -1) {  // no O_CREAT
        log_printf(FATAL, "shm_open: input_shm. %s", strerror(errno));
        exit(1);
    }
    if ((input_shm_ptr = mmap(NULL, sizeof(input_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
        log_printf(FATAL, "mmap: input_shm. %s", strerror(errno));
        exit(1);
    }
    if (close(fd_shm) == -1) {
        log_printf(ERROR, "close: input_shm. %s", strerror(errno));
    }

    // open robot desc shm block and map to client process virtual memory
    if ((fd_shm = shm_open(ROBOT_DESC_SHM_NAME, O_RDWR, 0)) == -1) {  // no O_CREAT
        log_printf(FATAL, "shm_open: robot_desc_shm. %s", strerror(errno));
        exit(1);
    }
    if ((rd_shm_ptr = mmap(NULL, sizeof(robot_desc_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
        log_printf(FATAL, "mmap: robot_desc_shm. %s", strerror(errno));
        exit(1);
    }
    if (close(fd_shm) == -1) {
        log_printf(ERROR, "close: robot_desc_shm. %s", strerror(errno));
    }

    // create log data shm block and map to client process virtual memory
    if ((fd_shm = shm_open(LOG_DATA_SHM, O_RDWR | O_CREAT, 0660)) == -1) {
        log_printf(FATAL, "shm_open log_data_shm: %s", strerror(errno));  // no O_CREAT
        exit(1);
    }
    if ((log_data_shm_ptr = mmap(NULL, sizeof(log_data_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
        log_printf(FATAL, "mmap log_data_shm: %s", strerror(errno));
        exit(1);
    }
    if (close(fd_shm) == -1) {
        log_printf(ERROR, "close log_data_shm: %s", strerror(errno));
    }

    atexit(shm_close);
}

void device_connect(dev_id_t* dev_id, int* dev_ix) {
    // wait on catalog_sem
    my_sem_wait(catalog_sem, "catalog_sem");

    // find a valid dev_ix
    for (*dev_ix = 0; *dev_ix < MAX_DEVICES; (*dev_ix)++) {
        if (!(dev_shm_ptr->catalog & (1 << *dev_ix))) {  // if the spot at dev_ix is free
            break;
        }
    }
    if (*dev_ix == MAX_DEVICES) {
        log_printf(ERROR, "device_connect: maximum device limit %d reached, connection refused", MAX_DEVICES);
        my_sem_post(catalog_sem, "catalog_sem");  // release the catalog semaphore
        *dev_ix = -1;
        return;
    }

    // wait on associated data and command sems
    my_sem_wait(sems[*dev_ix].data_sem, "data_sem");
    my_sem_wait(sems[*dev_ix].command_sem, "command_sem");

    // fill in dev_id for that device with provided values
    dev_shm_ptr->dev_ids[*dev_ix].type = dev_id->type;
    dev_shm_ptr->dev_ids[*dev_ix].year = dev_id->year;
    dev_shm_ptr->dev_ids[*dev_ix].uid = dev_id->uid;

    // update the catalog
    dev_shm_ptr->catalog |= (1 << *dev_ix);

    // reset param values to 0
    for (int i = 0; i < MAX_PARAMS; i++) {
        dev_shm_ptr->params[DATA][*dev_ix][i] = (const param_val_t){0};
        dev_shm_ptr->params[COMMAND][*dev_ix][i] = (const param_val_t){0};
    }

    // release associated data and command sems
    my_sem_post(sems[*dev_ix].data_sem, "data_sem");
    my_sem_post(sems[*dev_ix].command_sem, "command_sem");

    // release catalog_sem
    my_sem_post(catalog_sem, "catalog_sem");
}

void device_disconnect(int dev_ix) {
    // wait on catalog_sem
    my_sem_wait(catalog_sem, "catalog_sem");

    // wait on associated data and command sems
    my_sem_wait(sems[dev_ix].data_sem, "data_sem");
    my_sem_wait(sems[dev_ix].command_sem, "command_sem");
    my_sem_wait(cmd_map_sem, "cmd_map_sem");

    // update the catalog
    dev_shm_ptr->catalog &= (~(1 << dev_ix));

    // reset cmd bitmap values to 0
    dev_shm_ptr->cmd_map[0] &= (~(1 << dev_ix));  // reset the changed bit flag in cmd_map[0]
    dev_shm_ptr->cmd_map[dev_ix + 1] = 0;         // turn off all changed bits for the device

    my_sem_post(cmd_map_sem, "cmd_map_sem");
    // release associated upstream and downstream sems
    my_sem_post(sems[dev_ix].data_sem, "data_sem");
    my_sem_post(sems[dev_ix].command_sem, "command_sem");

    // release catalog_sem
    my_sem_post(catalog_sem, "catalog_sem");
}

int device_read(int dev_ix, process_t process, stream_t stream, uint32_t params_to_read, param_val_t* params) {
    // check catalog to see if dev_ix is valid, if not then return immediately
    if (!(dev_shm_ptr->catalog & (1 << dev_ix))) {
        log_printf(ERROR, "device_read: no device at dev_ix = %d, read failed", dev_ix);
        return -1;
    }

    // call the helper to do the actual reading
    device_read_helper(dev_ix, process, stream, params_to_read, params);
    return 0;
}

int device_read_uid(uint64_t dev_uid, process_t process, stream_t stream, uint32_t params_to_read, param_val_t* params) {
    int dev_ix;

    // if device doesn't exist, return immediately
    if ((dev_ix = get_dev_ix_from_uid(dev_uid)) == -1) {
        log_printf(ERROR, "device_read_uid: no device at dev_uid = %llu, read failed", dev_uid);
        return -1;
    }

    // call the helper to do the actual reading
    device_read_helper(dev_ix, process, stream, params_to_read, params);
    return 0;
}

int device_write(int dev_ix, process_t process, stream_t stream, uint32_t params_to_write, param_val_t* params) {
    // check catalog to see if dev_ix is valid, if not then return immediately
    if (!(dev_shm_ptr->catalog & (1 << dev_ix))) {
        log_printf(ERROR, "device_write: no device at dev_ix = %d, write failed", dev_ix);
        return -1;
    }

    // call the helper to do the actual reading
    device_write_helper(dev_ix, process, stream, params_to_write, params);
    return 0;
}

int device_write_uid(uint64_t dev_uid, process_t process, stream_t stream, uint32_t params_to_write, param_val_t* params) {
    int dev_ix;

    // if device doesn't exist, return immediately
    if ((dev_ix = get_dev_ix_from_uid(dev_uid)) == -1) {
        log_printf(ERROR, "device_write_uid: no device at dev_uid = %llu, write failed", dev_uid);
        return -1;
    }

    // call the helper to do the actual reading
    device_write_helper(dev_ix, process, stream, params_to_write, params);
    return 0;
}

void get_cmd_map(uint32_t bitmap[MAX_DEVICES + 1]) {
    // wait on cmd_map_sem
    my_sem_wait(cmd_map_sem, "cmd_map_sem");

    for (int i = 0; i < MAX_DEVICES + 1; i++) {
        bitmap[i] = dev_shm_ptr->cmd_map[i];
    }

    // release cmd_map_sem
    my_sem_post(cmd_map_sem, "cmd_map_sem");
}

void get_device_identifiers(dev_id_t dev_ids[MAX_DEVICES]) {
    // wait on catalog_sem
    my_sem_wait(catalog_sem, "catalog_sem");

    for (int i = 0; i < MAX_DEVICES; i++) {
        dev_ids[i] = dev_shm_ptr->dev_ids[i];
    }

    // release catalog_sem
    my_sem_post(catalog_sem, "catalog_sem");
}

void get_catalog(uint32_t* catalog) {
    // wait on catalog_sem
    my_sem_wait(catalog_sem, "catalog_sem");

    *catalog = dev_shm_ptr->catalog;

    // release catalog_sem
    my_sem_post(catalog_sem, "catalog_sem");
}

robot_desc_val_t robot_desc_read(robot_desc_field_t field) {
    robot_desc_val_t ret;

    // wait on rd_sem
    my_sem_wait(rd_sem, "robot_desc_mutex");

    // read the value out, and turn off the appropriate element
    ret = rd_shm_ptr->fields[field];

    // release rd_sem
    my_sem_post(rd_sem, "robot_desc_mutex");

    return ret;
}

void robot_desc_write(robot_desc_field_t field, robot_desc_val_t val) {
    // wait on rd_sem
    my_sem_wait(rd_sem, "robot_desc_mutex");

    robot_desc_val_t prev_val = rd_shm_ptr->fields[field];
    if (prev_val != val) {
        // write the val into the field
        rd_shm_ptr->fields[field] = val;

        /**
         * Edge case: If no inputs are connected during TELEOP, stop the robot
         * This is a safety precaution; a UserInput is expected to be connected during TELEOP,
         * so it's safe to assume something is wrong if there isn't one connected.
         * Note: We may be spammed with DISCONNECT, so this check should be done only when a change is made.
         * 
         * Example:
         *  Write to velocity in teleop_setup() while input connected, which will succeed
         *  Disconnect input
         *  velocity will still be nonzero, so we need to stop it
         */
        if (rd_shm_ptr->fields[RUN_MODE] == TELEOP && rd_shm_ptr->fields[KEYBOARD] == DISCONNECTED && rd_shm_ptr->fields[GAMEPAD] == DISCONNECTED) {
            log_printf(INFO, "Stopping Robot... (TELEOP with no input connected)");
            stop_robot();
        }
    }

    // release rd_sem
    my_sem_post(rd_sem, "robot_desc_mutex");
}

int input_read(uint64_t* pressed_buttons, float joystick_vals[4], robot_desc_field_t source) {
    if (source != GAMEPAD && source != KEYBOARD) {
        log_printf(FATAL, "input_read: incorrect API usage, can only read inputs for GAMEPAD and KEYBOARD.");
        exit(1);
    }

    // wait on rd_sem
    my_sem_wait(rd_sem, "robot_desc_mutex");

    // if input isn't connected, then release rd_sem and return
    if (rd_shm_ptr->fields[source] == DISCONNECTED) {
        my_sem_post(rd_sem, "robot_desc_mutex");
        return -1;
    }

    // release rd_sem
    my_sem_post(rd_sem, "robot_desc_mutex");

    // wait on gp_sem
    my_sem_wait(input_sem, "input_mutex");

    int index = (source == GAMEPAD) ? 0 : 1;
    *pressed_buttons = input_shm_ptr->inputs[index].buttons;
    if (source == GAMEPAD) {
        for (int i = 0; i < 4; i++) {
            joystick_vals[i] = input_shm_ptr->inputs[index].joysticks[i];
        }
    }


    // release gp_sem
    my_sem_post(input_sem, "input_mutex");

    return 0;
}

int input_write(uint64_t pressed_buttons, float joystick_vals[4], robot_desc_field_t source) {
    if (source != GAMEPAD && source != KEYBOARD) {
        log_printf(FATAL, "input_write: incorrect API usage, can only write inputs for GAMEPAD and KEYBOARD.");
        exit(1);
    }

    // wait on rd_sem
    my_sem_wait(rd_sem, "robot_desc_mutex");

    // if input isn't connected, then release rd_sem and return
    if (rd_shm_ptr->fields[source] == DISCONNECTED) {
        log_printf(ERROR, "input_write: no %s connected", field_to_string(source));
        my_sem_post(rd_sem, "robot_desc_mutex");
        return -1;
    }

    // release rd_sem
    my_sem_post(rd_sem, "robot_desc_mutex");

    // wait on gp_sem
    my_sem_wait(input_sem, "input_mutex");

    int index = (source == GAMEPAD) ? 0 : 1;
    input_shm_ptr->inputs[index].buttons = pressed_buttons;

    // if source == KEYBOARD, then joystick_vals = NULL, resulting in segfault
    if (source == GAMEPAD) {
        for (int i = 0; i < 4; i++) {
            input_shm_ptr->inputs[index].joysticks[i] = joystick_vals[i];
        }
    }

    // release gp_sem
    my_sem_post(input_sem, "input_mutex");

    return 0;
}

int log_data_write(char* key, param_type_t type, param_val_t value) {
    // wait on log_data_sem
    my_sem_wait(log_data_sem, "log_data_mutex");

    // find the index corresponding to this key in the log_data shm block
    int idx = 0;
    for (; idx < log_data_shm_ptr->num_params; idx++) {
        if (strcmp(key, log_data_shm_ptr->names[idx]) == 0) {
            break;
        }
    }

    // return if we ran out of keys for log data
    if (idx == UCHAR_MAX) {
        log_printf(ERROR, "Maximum number of %d log data keys reached. can't add key %s", UCHAR_MAX, key);
        return -1;
    }

    if (strlen(key) >= LOG_KEY_LENGTH) {
        log_printf(ERROR, "Key name %s for log data is longer than %d characters", key, LOG_KEY_LENGTH);
        return -2;
    }

    // copy over the name, type, and parameter of the log data into the shared memory block
    strcpy(log_data_shm_ptr->names[idx], key);
    log_data_shm_ptr->types[idx] = type;
    log_data_shm_ptr->params[idx] = value;

    // if this a new parameter, increment the number of parameters in log data shared memory block
    if (idx == log_data_shm_ptr->num_params) {
        log_data_shm_ptr->num_params++;
    }

    // release log_data_sem
    my_sem_post(log_data_sem, "log_data_mutex");

    return 0;
}

void log_data_read(uint8_t* num_params, char names[UCHAR_MAX][LOG_KEY_LENGTH], param_type_t types[UCHAR_MAX], param_val_t values[UCHAR_MAX]) {
    // wait on log_data_sem
    my_sem_wait(log_data_sem, "log_data_mutex");

    // read all of the data in the log data shared memory block into provided pointers
    *num_params = log_data_shm_ptr->num_params;
    for (int i = 0; i < *num_params; i++) {
        strcpy(names[i], log_data_shm_ptr->names[i]);
        types[i] = log_data_shm_ptr->types[i];
        values[i] = log_data_shm_ptr->params[i];
    }

    // release log_data_sem
    my_sem_post(log_data_sem, "log_data_mutex");
}
