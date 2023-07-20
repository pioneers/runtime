#ifndef SHM_CLIENT_H
#define SHM_CLIENT_H

#include <logger.h>  // for log_printf
#include <shm_wrapper.h>
#include <sys/wait.h>

/**
 * Creates and initializes the shared memory process
 */
void start_shm();

/**
 * Unlinks/destroys the shared memory and kills the shared memory process spawned by start_shm
 */
void stop_shm();

#endif
