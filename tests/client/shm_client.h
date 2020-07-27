#ifndef SHM_CLIENT_H
#define SHM_CLIENT_H

#include "../../shm_wrapper/shm_wrapper.h"
#include <sys/wait.h>

/*
 * Creates and initializes the shared memory process
 */
void start_shm ();

/*
 * Unlinks/destroys the shared memory and kills the shared memory process spawned by start_shm
 */
void stop_shm ();

/*
 * Print the current state of the shared memory to the standard output of the calling process
 * Uses the printing utility functions in shm_wrapper.h to do this
 */
void print_shm ();

#endif
