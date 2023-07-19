#ifndef SHM_CLIENT_H
#define SHM_CLIENT_H

#include <sys/wait.h>
#include "../../logger/logger.h"  // for log_printf
#include "../../shm_wrapper/shm_wrapper.h"

/**
 * Creates and initializes the shared memory process
 */
void start_shm();

/**
 * Unlinks/destroys the shared memory and kills the shared memory process spawned by start_shm
 */
void stop_shm();

#endif
