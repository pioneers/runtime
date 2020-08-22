#include "shm_wrapper.h"

char sname[SNAME_SIZE];  // being lazy here but this is for holding all the semaphore names

// *********************************** SHM PROCESS UTILITIES *********************************************** //

/**
 * Unlinks a block of shared memory (so it will be removed from the system once all processes call shm_close() on it)
 * Arguments:
 *    shm_name: name of shared memory block (ex. "/dev-shm")
 *    shm_desc: description of shared memory block, to be printed with descriptive error message should shm_unlink() fail
 */
static void my_shm_unlink(char* shm_name, char* shm_desc) {
    if (shm_unlink(shm_name) == -1) {
        log_printf(ERROR, "shm_unlink: %s. %s", shm_desc, strerror(errno));
    }
}

/**
 * Unlinks a semaphore (so it will be removed from the system once all processes call sem_close() on it)
 * Arguments:
 *    sem_name: name of semaphore (ex. "/ct-mutex")
 *    sem_desc: description of semaphore, to be printed with descriptive error message should sem_unlink() fail
 */
static void my_sem_unlink(char* sem_name, char* sem_desc) {
    if (sem_unlink(sem_name) == -1) {
        log_printf(ERROR, "sem_unlink: %s. %s", sem_desc, strerror(errno));
    }
}

/**
 * This program unlinks all of the shared memory blocks and associated semaphores.
 * Must run after net_handler, dev_handler, or executor terminate during reboot
 */
int main() {
    // init the logger and shared memory
    logger_init(SHM);
    shm_init();

    // unlink all shared memory blocks
    my_shm_unlink(DEV_SHM_NAME, "dev_shm");
    my_shm_unlink(GPAD_SHM_NAME, "gamepad_shm");
    my_shm_unlink(ROBOT_DESC_SHM_NAME, "robot_desc_shm");
    my_shm_unlink(LOG_DATA_SHM, "log_data_shm");

    // unlink all semaphores
    my_sem_unlink(CATALOG_MUTEX_NAME, "catalog mutex");
    my_sem_unlink(CMDMAP_MUTEX_NAME, "cmd map mutex");
    my_sem_unlink(SUBMAP_MUTEX_NAME, "sub map mutex");
    my_sem_unlink(GP_MUTEX_NAME, "gamepad mutex");
    my_sem_unlink(RD_MUTEX_NAME, "robot desc mutex");
    my_sem_unlink(LOG_DATA_MUTEX, "log data mutex");
    for (int i = 0; i < MAX_DEVICES; i++) {
        generate_sem_name(DATA, i, sname);
        if (sem_unlink((const char*) sname) == -1) {
            log_printf(ERROR, "sem_unlink: data_sem for dev_ix %d. %s", i, strerror(errno));
        }
        generate_sem_name(COMMAND, i, sname);
        if (sem_unlink((const char*) sname) == -1) {
            log_printf(ERROR, "sem_unlink: command_sem for dev_ix %d. %s", i, strerror(errno));
        }
    }

    // Using shm_init() calls shm_stop() automatically on process exit, so semaphores and shm blocks will be closed on exit

    log_printf(INFO, "SHM destroyed. Runtime Funtime is no more");

    /*
     * Under normal production circumstances, this program should never be run (it only runs when net_handler, executor, or dev_handler
     * crash abnormally). So, systemd needs to know that this process "failed" so that it can run shm_start to try and boot up Runtime
     * again. A nonzero exit status is a failure, so we return 1 here to tell systemd that this process "failed".
     */
    return 1;
}
