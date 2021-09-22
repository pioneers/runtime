#include "shm_wrapper.h"

char sname[SNAME_SIZE];  // being lazy here but this is for holding all the semaphore names

// ************************************ SHM UTILITY *********************************************** //

/**
 * Custom wrapper function for sem_open with create flag. Prints out descriptive logging message on failure
 * exits (not being able to create and open a semaphore is fatal to Runtime).
 * Arguments:
 *    sem_name: name of a semaphore to be created and opened
 *    sem_desc: string that describes the semaphore being created and opened, displayed with error message
 * Returns a pointer to the semaphore that was created and opened.
 */
static sem_t* my_sem_open_create(char* sem_name, char* sem_desc) {
    sem_t* ret;
    if ((ret = sem_open(sem_name, O_CREAT, 0660, 1)) == SEM_FAILED) {
        log_printf(FATAL, "sem_open: %s. %s", sem_desc, strerror(errno));
        exit(1);
    }
    return ret;
}

/**
 * This program creates and opens all of the shared memory blocks and associated semaphores.
 * Must run before net_handler, dev_handler, or executor connect to it.
 * Initializes shared memory blocks to default values.
 */
int main() {
    // set up
    logger_init(SHM);

    int fd_shm;

    // create all semaphores with initial value 1
    catalog_sem = my_sem_open_create(CATALOG_MUTEX_NAME, "catalog mutex");
    cmd_map_sem = my_sem_open_create(CMDMAP_MUTEX_NAME, "cmd map mutex");
    input_sem = my_sem_open_create(INPUTS_MUTEX_NAME, "inputs mutex");
    rd_sem = my_sem_open_create(RD_MUTEX_NAME, "robot desc mutex");
    log_data_sem = my_sem_open_create(LOG_DATA_MUTEX, "log data mutex");
    for (int i = 0; i < MAX_DEVICES; i++) {
        generate_sem_name(DATA, i, sname);  // get the data name
        if ((sems[i].data_sem = sem_open((const char*) sname, O_CREAT, 0660, 1)) == SEM_FAILED) {
            log_printf(FATAL, "sem_open: data sem for dev_ix %d: %s", i, strerror(errno));
            exit(1);
        }
        generate_sem_name(COMMAND, i, sname);  // get the command name
        if ((sems[i].command_sem = sem_open((const char*) sname, O_CREAT, 0660, 1)) == SEM_FAILED) {
            log_printf(FATAL, "sem_open: command sem for dev_ix %d: %s", i, strerror(errno));
            exit(1);
        }
    }

    // create device shm block
    if ((fd_shm = shm_open(DEV_SHM_NAME, O_RDWR | O_CREAT, 0660)) == -1) {
        log_printf(FATAL, "shm_open devices: %s", strerror(errno));
        exit(1);
    }
    if (ftruncate(fd_shm, sizeof(dev_shm_t)) == -1) {
        log_printf(FATAL, "ftruncate devices: %s", strerror(errno));
        exit(1);
    }
    if ((dev_shm_ptr = mmap(NULL, sizeof(dev_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
        log_printf(FATAL, "mmap devices: %s", strerror(errno));
        exit(1);
    }
    if (close(fd_shm) == -1) {
        log_printf(ERROR, "close devices: %s", strerror(errno));
    }

    // create inputs shm block
    if ((fd_shm = shm_open(INPUTS_SHM_NAME, O_RDWR | O_CREAT, 0660)) == -1) {
        log_printf(FATAL, "shm_open input_shm: %s", strerror(errno));
        exit(1);
    }
    if (ftruncate(fd_shm, sizeof(input_shm_t)) == -1) {
        log_printf(FATAL, "ftruncate input_shm: %s", strerror(errno));
        exit(1);
    }
    if ((input_shm_ptr = mmap(NULL, sizeof(input_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
        log_printf(FATAL, "mmap input_shm: %s", strerror(errno));
        exit(1);
    }
    if (close(fd_shm) == -1) {
        log_printf(ERROR, "close input_shm: %s", strerror(errno));
    }

    // create robot description shm block
    if ((fd_shm = shm_open(ROBOT_DESC_SHM_NAME, O_RDWR | O_CREAT, 0660)) == -1) {
        log_printf(FATAL, "shm_open robot_desc_shm: %s", strerror(errno));
        exit(1);
    }
    if (ftruncate(fd_shm, sizeof(robot_desc_shm_t)) == -1) {
        log_printf(FATAL, "ftruncate robot_desc_shm: %s", strerror(errno));
        exit(1);
    }
    if ((rd_shm_ptr = mmap(NULL, sizeof(robot_desc_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
        log_printf(FATAL, "mmap robot_desc_shm: %s", strerror(errno));
        exit(1);
    }
    if (close(fd_shm) == -1) {
        log_printf(ERROR, "close robot_desc_shm: %s", strerror(errno));
    }

    // create log data shm block
    if ((fd_shm = shm_open(LOG_DATA_SHM, O_RDWR | O_CREAT, 0660)) == -1) {
        log_printf(FATAL, "shm_open log_data_shm: %s", strerror(errno));
        exit(1);
    }
    if (ftruncate(fd_shm, sizeof(log_data_shm_t)) == -1) {
        log_printf(FATAL, "ftruncate log_data_shm: %s", strerror(errno));
        exit(1);
    }
    if ((log_data_shm_ptr = mmap(NULL, sizeof(log_data_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
        log_printf(FATAL, "mmap log_data_shm: %s", strerror(errno));
        exit(1);
    }
    if (close(fd_shm) == -1) {
        log_printf(ERROR, "close log_data_shm: %s", strerror(errno));
    }

    // initialize everything
    dev_shm_ptr->catalog = 0;
    for (int i = 0; i < MAX_DEVICES + 1; i++) {
        dev_shm_ptr->cmd_map[i] = 0;
    }
    for (int j = 0; j < 2; j++) {
        input_shm_ptr->inputs[j].buttons = 0;
        for (int i = 0; i < 4; i++) {
            input_shm_ptr->inputs[j].joysticks[i] = 0.0;
        }
    }
    input_shm_ptr->inputs[0].source = GAMEPAD;
    input_shm_ptr->inputs[1].source = KEYBOARD;
    rd_shm_ptr->fields[RUN_MODE] = IDLE;
    rd_shm_ptr->fields[DAWN] = DISCONNECTED;
    rd_shm_ptr->fields[SHEPHERD] = DISCONNECTED;
    rd_shm_ptr->fields[GAMEPAD] = DISCONNECTED;
    rd_shm_ptr->fields[KEYBOARD] = DISCONNECTED;
    rd_shm_ptr->fields[START_POS] = LEFT;

    memset(log_data_shm_ptr, 0, sizeof(log_data_shm_t));

    log_printf(INFO, "SHM created");

    return 0;  // returns to start everything
}
