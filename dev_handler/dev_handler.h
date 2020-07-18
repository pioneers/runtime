/**
 * Handles lowcar device connects/disconnects and acts as the interface between the devices and shared memory
 */

#ifndef DEV_HANDLER_H
#define DEV_HANDLER_H

#include "../shm_wrapper/shm_wrapper.h"
#include "message.h"
#include "../runtime_util/runtime_util.h"
#include "../logger/logger.h"

#include <pthread.h>
#include <termios.h>    // POSIX terminal control definitions in serialport_init()
#include <stdio.h>      // Print
#include <stdlib.h>
#include <stdint.h>     // ints with specified sizes (uint8_t, uint16_t, etc.)
#include <signal.h>     // Used to handle SIGTERM, SIGINT, SIGKILL
#include <sys/time.h>   // For timestamps on PING
#include <unistd.h>     // sleep(int seconds)

// ************************************ CONFIG ************************************* //

// The types of output the DEV_HANDLER can communicate with. FILE_DEV is used for `make fake`
typedef enum { USB_DEV, FILE_DEV } output_type_t;

// Whether DEV_HANDLER should communicate with USB devices over serial or to a fake device over .txt files (for testing)
#define OUTPUT USB_DEV // Choose USB_DEV when testing with Arduinos. Choose FILE_DEV when using `make fake`

// The file to write to if output == FILE_DEV
#define TO_DEVICE "to_device.txt"

// The file to read from if output == FILE_DEV
#define TO_DEV_HANDLER "to_dev_handler.txt"

// ************************************ STARTING/STOPING LIBUSB AND SHARED MEMORY ************************************ //

/*
 * Initializes data structures, libusb, and shared memory
 */
void init();

/*
 * Frees data structures, stops libusb, and stops shared memory
 */
void stop();

// ************************************ POLLING FOR DEVICES ************************************ //

/*
 * Detects when devices are connected and disconnected.
 * On lowcar device connect, connect to shared memory and spawn three threads to communicate with the device
 * On lowcar device disconnect, disconnects the device from shared memory.
 */
void poll_connected_devices();

/* The maximum number of milliseconds to wait between each PING from a device
 * Waiting for this long will exit all threads for that device (doing cleanup as necessary) */
#define TIMEOUT 2500

/* The number of milliseconds between each PING sent to the device */
#define PING_FREQ 1000

#endif
