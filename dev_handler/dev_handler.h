/**
 * Handles lowcar device connects/disconnects and acts as the interface between the devices and shared memory
 */

#ifndef DEV_HANDLER_H
#define DEV_HANDLER_H

#include "../shm_wrapper/shm_wrapper.h"
#include "message.h"
#include "../runtime_util/runtime_util.h"
#include "../logger/logger.h"

#include <pthread.h>    // sender, receiver, and relayer threads
#include <termios.h>    // POSIX terminal control definitions in serialport_init()
#include <stdio.h>      // Print
#include <stdlib.h>
#include <stdint.h>     // ints with specified sizes (uint8_t, uint16_t, etc.)
#include <signal.h>     // Used to handle SIGINT (Ctrl+C)
#include <sys/time.h>   // For timestamps on PING
#include <unistd.h>     // sleep(int seconds)

// ************************************ CONFIG ************************************* //

/* The maximum number of milliseconds to wait between each PING from a device
 * Waiting for this long will exit all threads for that device (doing cleanup as necessary) */
#define TIMEOUT 2500

/* The number of milliseconds between each PING sent to the device */
#define PING_FREQ 1000

// ************************************ PUBLIC FUNCTIONS ************************************* //

// Initialize logger, shm, and mutexes
void init();

// Disconnect devices from shared memory and destroy mutexes
void stop();

/**
 * Detects when devices are connected
 * On Arduino device connect, connect to shared memory and spawn three threads to communicate with the device
 */
void poll_connected_devices();

#endif
