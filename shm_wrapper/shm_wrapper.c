#include "shm_wrapper.h"

// *********************************** WRAPPER-SPECIFIC GLOBAL VARS **************************************** //

dual_sem_t sems[MAX_DEVICES];  // array of semaphores, two for each possible device (one for data and one for commands)
dev_shm_t* dev_shm_ptr;        // points to memory-mapped shared memory block for device data and commands
sem_t* catalog_sem;            // semaphore used as a mutex on the catalog
sem_t* cmd_map_sem;            // semaphore used as a mutex on the command bitmap
sem_t* sub_map_sem;            // semaphore used as a mutex on the subscription bitmap

gamepad_shm_t* gp_shm_ptr;     // points to memory-mapped shared memory block for gamepad
robot_desc_shm_t* rd_shm_ptr;  // points to memory-mapped shared memory block for robot description
sem_t* gp_sem;                 // semaphore used as a mutex on the gamepad
sem_t* rd_sem;                 // semaphore used as a mutex on the robot description

log_data_shm_t* log_data_shm_ptr;  // points to shared memory block for log data specified by executor
sem_t* log_data_sem;               // semaphore used as a mutex on the log data

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
 * Function that prints a bitmap in human-readable way (in binary, with literal 1s and 0s)
 * Arguments:
 *    num_bits: number of bits to print (max 32 bitS, although that can be increased by changing the size of BITMAP)
 *    bitmap: bitmap to print
 */
static void print_bitmap(int num_bits, uint32_t bitmap) {
    for (int i = 0; i < num_bits; i++) {
        printf("%d", (bitmap & (1 << i)) ? 1 : 0);
    }
    printf("\n");
}

/**
 * Function that returns dev_ix of specified device if it exists (-1 if it doesn't)
 * Arguments:
 *    dev_uid: 64-bit unique ID of the device
 * Returns: device index in shared memory of the specified device, -1 if specified device is not in shared memory
 */
static int get_dev_ix_from_uid(uint64_t dev_uid) {
    int dev_ix = -1;

    for (int i = 0; i < MAX_DEVICES; i++) {
        if ((dev_shm_ptr->catalog & (1 << i)) && (dev_shm_ptr->dev_ids[i].uid == dev_uid)) {
            dev_ix = i;
            break;
        }
    }
    return dev_ix;
}

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

    // turn on flag if executor is sending a command to the device
    // if stream = downstream and process = executor then also update params bitmap
    if (process == EXECUTOR && stream == COMMAND) {
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
    my_sem_close(sub_map_sem, "sub map sem");
    my_sem_close(gp_sem, "gamepad_mutex");
    my_sem_close(rd_sem, "robot_desc_mutex");
    my_sem_close(log_data_sem, "log data mutex");

    // unmap all shared memory blocks
    if (munmap(dev_shm_ptr, sizeof(dev_shm_t)) == -1) {
        log_printf(ERROR, "munmap: dev_shm. %s", strerror(errno));
    }
    if (munmap(gp_shm_ptr, sizeof(gamepad_shm_t)) == -1) {
        log_printf(ERROR, "munmap: gp_shm. %s", strerror(errno));
    }
    if (munmap(rd_shm_ptr, sizeof(robot_desc_shm_t)) == -1) {
        log_printf(ERROR, "munmap: robot_desc_shm. %s", strerror(errno));
    }
    if (munmap(log_data_shm_ptr, sizeof(log_data_shm_t)) == -1) {
        log_printf(ERROR, "munmap: log_data_shm_ptr. %s", strerror(errno));
    }
}

// ************************************ PUBLIC PRINTING UTILITIES ***************************************** //

void print_cmd_map() {
    uint32_t cmd_map[MAX_DEVICES + 1];
    get_cmd_map(cmd_map);

    printf("Changed devices: ");
    print_bitmap(MAX_DEVICES, cmd_map[0]);

    printf("Changed params:\n");
    for (int i = 1; i < 33; i++) {
        if (cmd_map[0] & (1 << (i - 1))) {
            printf("\tDevice %d: ", i - 1);
            print_bitmap(MAX_PARAMS, cmd_map[i]);
        }
    }
}

void print_sub_map(process_t process) {
    uint32_t sub_map[MAX_DEVICES + 1];
    get_sub_requests(sub_map, process);

    printf("Requested devices: ");
    print_bitmap(MAX_DEVICES, sub_map[0]);

    printf("Requested params:\n");
    for (int i = 1; i < 33; i++) {
        if (sub_map[0] & (1 << (i - 1))) {
            printf("\tDevice %d: ", i - 1);
            print_bitmap(MAX_PARAMS, sub_map[i]);
        }
    }
}

void print_dev_ids() {
    dev_id_t dev_ids[MAX_DEVICES];
    uint32_t catalog;

    get_device_identifiers(dev_ids);
    get_catalog(&catalog);

    if (catalog == 0) {
        printf("no connected devices\n");
    } else {
        for (int i = 0; i < MAX_DEVICES; i++) {
            if (catalog & (1 << i)) {
                printf("dev_ix = %d: type = %d, year = %d, uid = %llu\n", i, dev_ids[i].type, dev_ids[i].year, dev_ids[i].uid);
            }
        }
    }
}

void print_catalog() {
    uint32_t catalog;

    get_catalog(&catalog);
    print_bitmap(MAX_DEVICES, catalog);
}

void print_params(uint32_t devices) {
    dev_id_t dev_ids[MAX_DEVICES];
    uint32_t catalog;
    device_t* device;

    get_device_identifiers(dev_ids);
    get_catalog(&catalog);

    for (int i = 0; i < MAX_DEVICES; i++) {
        if ((catalog & (1 << i)) && (devices & (1 << i))) {
            device = get_device(dev_ids[i].type);
            if (device == NULL) {
                printf("Device at index %d with type %d is invalid\n", i, dev_ids[i].type);
                continue;
            }
            printf("dev_ix = %d: name = %s, type = %d, year = %d, uid = %llu\n", i, device->name, dev_ids[i].type, dev_ids[i].year, dev_ids[i].uid);

            for (int s = 0; s < 2; s++) {
                //print out the stream header
                if (s == 0) {
                    printf("\tDATA stream:\n");
                } else if (s == 1) {
                    printf("\tCOMMAND stream:\n");
                }

                //print all params for the device for that stream
                for (int j = 0; j < device->num_params; j++) {
                    switch (device->params[j].type) {
                        case INT:
                            printf("\t\tparam_idx = %d, name = %s, value = %d\n", j, device->params[j].name, dev_shm_ptr->params[s][i][j].p_i);
                            break;
                        case FLOAT:
                            printf("\t\tparam_idx = %d, name = %s, value = %f\n", j, device->params[j].name, dev_shm_ptr->params[s][i][j].p_f);
                            break;
                        case BOOL:
                            printf("\t\tparam_idx = %d, name = %s, value = %s\n", j, device->params[j].name, (dev_shm_ptr->params[s][i][j].p_b) ? "True" : "False");
                            break;
                    }
                }
            }
        }
    }
}

void print_robot_desc() {
    // wait on rd_sem
    my_sem_wait(rd_sem, "robot_desc_mutex (in print)");

    printf("Current Robot Description:\n");
    for (int i = 0; i < NUM_DESC_FIELDS; i++) {
        switch (i) {
            case RUN_MODE:
                printf("\tRUN_MODE = %s\n", (rd_shm_ptr->fields[RUN_MODE] == IDLE) ? "IDLE" : ((rd_shm_ptr->fields[RUN_MODE] == AUTO) ? "AUTO" : ((rd_shm_ptr->fields[RUN_MODE] == TELEOP) ? "TELEOP" : "CHALLENGE")));
                break;
            case DAWN:
                printf("\tDAWN = %s\n", (rd_shm_ptr->fields[DAWN] == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
                break;
            case SHEPHERD:
                printf("\tSHEPHERD = %s\n", (rd_shm_ptr->fields[SHEPHERD] == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
                break;
            case GAMEPAD:
                printf("\tGAMEPAD = %s\n", (rd_shm_ptr->fields[GAMEPAD] == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
                break;
            case START_POS:
                printf("\tSTART_POS = %s\n", (rd_shm_ptr->fields[START_POS] == LEFT) ? "LEFT" : "RIGHT");
                break;
            default:
                printf("unknown field\n");
        }
    }
    printf("\n");
    fflush(stdout);

    //release rd_sem
    my_sem_post(rd_sem, "robot_desc_mutex (in print)");
}

void print_gamepad_state() {
    char** button_names = get_button_names();
    char** joystick_names = get_joystick_names();
    uint32_t buttons;
    float joysticks[4];

    printf("Current Gamepad State:\n");
    if (gamepad_read(&buttons, joysticks) == -1) {
        printf("\tNo gamepad currently connected\n");
    } else {
        printf("\tPushed Buttons:\n");
        // only print pushed buttons (so we don't print out many lines of output each time we all this function)
        for (int i = 0; i < NUM_GAMEPAD_BUTTONS; i++) {
            if (buttons & (1 << i)) {
                printf("\t\t%s\n", button_names[i]);
            }
        }
        printf("\tJoystick Positions:\n");
        // print joystick positions
        for (int i = 0; i < 4; i++) {
            printf("\t\t%s = %f\n", joystick_names[i], joysticks[i]);
        }
        printf("\n");
    }
    fflush(stdout);
}

void print_custom_data() {
    // wait on log_data_sem
    my_sem_wait(log_data_sem, "log_data_mutex (in print)");

    // print out contents of custom log data shared memory block
    printf("Custom logged data (Device %d):\n", MAX_DEVICES);
    for (int i = 0; i < log_data_shm_ptr->num_params; i++) {
        printf("\t%s: ", log_data_shm_ptr->names[i]);
        switch (log_data_shm_ptr->types[i]) {
            case INT:
                printf("%d\n", log_data_shm_ptr->params[i].p_i);
                break;
            case FLOAT:
                printf("%f\n", log_data_shm_ptr->params[i].p_f);
                break;
            case BOOL:
                printf("%s\n", log_data_shm_ptr->params[i].p_b == 1 ? "True" : "False");
                break;
        }
    }

    // release log_data_sem
    my_sem_post(log_data_sem, "log_data_mutex (in print)");
}

// ************************************ PUBLIC WRAPPER FUNCTIONS ****************************************** //

void generate_sem_name(stream_t stream, int dev_ix, char* name) {
    if (stream == DATA) {
        sprintf(name, "/data_sem_%d", dev_ix);
    } else if (stream == COMMAND) {
        sprintf(name, "/command_sem_%d", dev_ix);
    }
}

void shm_init() {
    int fd_shm;              // file descriptor of the memory-mapped shared memory
    char sname[SNAME_SIZE];  // for holding semaphore names

    // open all the semaphores
    catalog_sem = my_sem_open(CATALOG_MUTEX_NAME, "catalog mutex");
    cmd_map_sem = my_sem_open(CMDMAP_MUTEX_NAME, "cmd map mutex");
    sub_map_sem = my_sem_open(SUBMAP_MUTEX_NAME, "sub map mutex");
    gp_sem = my_sem_open(GP_MUTEX_NAME, "gamepad mutex");
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
        log_printf(FATAL, "shm_open dev_shm %s", strerror(errno));
        exit(1);
    }
    if ((dev_shm_ptr = mmap(NULL, sizeof(dev_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
        log_printf(FATAL, "mmap dev_shm: %s", strerror(errno));
        exit(1);
    }
    if (close(fd_shm) == -1) {
        log_printf(ERROR, "close dev_shm %s", strerror(errno));
    }

    // open gamepad shm block and map to client process virtual memory
    if ((fd_shm = shm_open(GPAD_SHM_NAME, O_RDWR, 0)) == -1) {  // no O_CREAT
        log_printf(FATAL, "shm_open: gamepad_shm. %s", strerror(errno));
        exit(1);
    }
    if ((gp_shm_ptr = mmap(NULL, sizeof(gamepad_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
        log_printf(FATAL, "mmap: gamepad_shm. %s", strerror(errno));
        exit(1);
    }
    if (close(fd_shm) == -1) {
        log_printf(ERROR, "close: gamepad_shm. %s", strerror(errno));
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

void device_connect(dev_id_t dev_id, int* dev_ix) {
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
    my_sem_wait(sub_map_sem, "sub_map_sem");

    // fill in dev_id for that device with provided values
    dev_shm_ptr->dev_ids[*dev_ix].type = dev_id.type;
    dev_shm_ptr->dev_ids[*dev_ix].year = dev_id.year;
    dev_shm_ptr->dev_ids[*dev_ix].uid = dev_id.uid;

    // update the catalog
    dev_shm_ptr->catalog |= (1 << *dev_ix);

    // reset param values to 0
    for (int i = 0; i < MAX_PARAMS; i++) {
        dev_shm_ptr->params[DATA][*dev_ix][i] = (const param_val_t){0};
        dev_shm_ptr->params[COMMAND][*dev_ix][i] = (const param_val_t){0};
    }

    // reset executor subscriptions to off
    dev_shm_ptr->exec_sub_map[0] &= ~(1 << *dev_ix);
    dev_shm_ptr->exec_sub_map[*dev_ix + 1] = 0;

    // reset net_handler subscriptions to on
    dev_shm_ptr->net_sub_map[0] |= 1 << *dev_ix;
    dev_shm_ptr->net_sub_map[*dev_ix + 1] = -1;

    my_sem_post(sub_map_sem, "sub_map_sem");
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
        log_printf(ERROR, "no device at dev_ix = %d, read failed", dev_ix);
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
        log_printf(ERROR, "no device at dev_uid = %llu, read failed", dev_uid);
        return -1;
    }

    // call the helper to do the actual reading
    device_read_helper(dev_ix, process, stream, params_to_read, params);
    return 0;
}

int device_write(int dev_ix, process_t process, stream_t stream, uint32_t params_to_write, param_val_t* params) {
    // check catalog to see if dev_ix is valid, if not then return immediately
    if (!(dev_shm_ptr->catalog & (1 << dev_ix))) {
        log_printf(ERROR, "no device at dev_ix = %d, write failed", dev_ix);
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
        log_printf(ERROR, "no device at dev_uid = %llu, write failed", dev_uid);
        return -1;
    }

    // call the helper to do the actual reading
    device_write_helper(dev_ix, process, stream, params_to_write, params);
    return 0;
}

int place_sub_request(uint64_t dev_uid, process_t process, uint32_t params_to_sub) {
    int dev_ix;              // dev_ix that we're operating on
    uint32_t* curr_sub_map;  // sub map that we're operating on

    // validate request and obtain dev_ix, sub_map
    if (process != NET_HANDLER && process != EXECUTOR) {
        log_printf(ERROR, "calling place_sub_request from process %u", process);
        return -1;
    }
    if ((dev_ix = get_dev_ix_from_uid(dev_uid)) == -1) {
        log_printf(ERROR, "no device at dev_uid = %llu, sub request failed", dev_uid);
        return -1;
    }
    curr_sub_map = (process == NET_HANDLER) ? dev_shm_ptr->net_sub_map : dev_shm_ptr->exec_sub_map;

    // wait on sub_map_sem
    my_sem_wait(sub_map_sem, "sub_map_sem");

    // only fill in params_to_sub if it's different from what's already there
    if (curr_sub_map[dev_ix + 1] != params_to_sub) {
        curr_sub_map[dev_ix + 1] = params_to_sub;
        curr_sub_map[0] |= (1 << dev_ix);  // turn on corresponding bit
    }
    
    // release sub_map_sem
    my_sem_post(sub_map_sem, "sub_map_sem");

    return 0;
}

void get_sub_requests(uint32_t sub_map[MAX_DEVICES + 1], process_t process) {
    // wait on sub_map_sem
    my_sem_wait(sub_map_sem, "sub_map_sem");

    // bitwise OR the changes into sub_map[0]; if no changes then return
    if (process == NET_HANDLER) {
        sub_map[0] = dev_shm_ptr->net_sub_map[0];
    } else if (process == EXECUTOR) {
        sub_map[0] = dev_shm_ptr->exec_sub_map[0];
    } else {
        sub_map[0] = dev_shm_ptr->net_sub_map[0] | dev_shm_ptr->exec_sub_map[0];
    }

    if (sub_map[0] == 0 && process == DEV_HANDLER) {
        my_sem_post(sub_map_sem, "sub_map_sem");
        return;
    }

    // bitwise OR each valid element together into sub_map[i]
    for (int i = 1; i < MAX_DEVICES + 1; i++) {
        if (dev_shm_ptr->catalog & (1 << (i - 1))) {  // if device exists
            if (process == NET_HANDLER) {
                sub_map[i] = dev_shm_ptr->net_sub_map[i];
            } else if (process == EXECUTOR) {
                sub_map[i] = dev_shm_ptr->exec_sub_map[i];
            } else {
                sub_map[i] = dev_shm_ptr->net_sub_map[i] | dev_shm_ptr->exec_sub_map[i];
            }
        } else {
            sub_map[0] &= ~(1 << (i - 1));
            sub_map[i] = 0;
        }
    }

    if (process == DEV_HANDLER) {
        // reset the change indicator eleemnts in net_sub_map and exec_sub_map
        dev_shm_ptr->net_sub_map[0] = dev_shm_ptr->exec_sub_map[0] = 0;
    }

    // release sub_map_sem
    my_sem_post(sub_map_sem, "sub_map_sem");
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

    // write the val into the field, and set appropriate pending element to 1
    rd_shm_ptr->fields[field] = val;

    // release rd_sem
    my_sem_post(rd_sem, "robot_desc_mutex");
}

int gamepad_read(uint32_t* pressed_buttons, float joystick_vals[4]) {
    // wait on rd_sem
    my_sem_wait(rd_sem, "robot_desc_mutex");

    // if no gamepad connected, then release rd_sem and return
    if (rd_shm_ptr->fields[GAMEPAD] == DISCONNECTED) {
        *pressed_buttons = 0;
        for (int i = 0; i < 4; i++) {
            joystick_vals[i] = 0;
        }
        log_printf(DEBUG, "tried to read, but no gamepad connected");
        my_sem_post(rd_sem, "robot_desc_mutex");
        return -1;
    }

    // release rd_sem
    my_sem_post(rd_sem, "robot_desc_mutex");

    // wait on gp_sem
    my_sem_wait(gp_sem, "gamepad_mutex");

    *pressed_buttons = gp_shm_ptr->buttons;
    for (int i = 0; i < 4; i++) {
        joystick_vals[i] = gp_shm_ptr->joysticks[i];
    }

    // release gp_sem
    my_sem_post(gp_sem, "gamepad_mutex");

    return 0;
}

int gamepad_write(uint32_t pressed_buttons, float joystick_vals[4]) {
    // wait on rd_sem
    my_sem_wait(rd_sem, "robot_desc_mutex");

    // if no gamepad connected, then release rd_sem and return
    if (rd_shm_ptr->fields[GAMEPAD] == DISCONNECTED) {
        log_printf(ERROR, "tried to write, but no gamepad connected");
        my_sem_post(rd_sem, "robot_desc_mutex");
        return -1;
    }

    // release rd_sem
    my_sem_post(rd_sem, "robot_desc_mutex");

    // wait on gp_sem
    my_sem_wait(gp_sem, "gamepad_mutex");

    gp_shm_ptr->buttons = pressed_buttons;
    for (int i = 0; i < 4; i++) {
        gp_shm_ptr->joysticks[i] = joystick_vals[i];
    }

    // release gp_sem
    my_sem_post(gp_sem, "gamepad_mutex");

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

    // return if we didn't find the log data corresponding to this key
    if (idx == UCHAR_MAX) {
        log_printf(ERROR, "Maximum number of %d log data keys reached. can't add key %s", UCHAR_MAX, key);
        return -1;
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

void log_data_read(uint8_t* num_params, char names[UCHAR_MAX][64], param_type_t types[UCHAR_MAX], param_val_t values[UCHAR_MAX]) {
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
