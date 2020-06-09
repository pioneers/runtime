/*
 * Handles lowcar device connects/disconnects and acts as the interface between the devices and shared memory
 * Requires third-party library libusb:
 *  API: http://libusb.sourceforge.net/api-1.0/libusb_api.html
 * Linux: sudo apt-get install libusb-1.0-0-dev
 *		  gcc -I/usr/include/libusb-1.0 dev_handler.c -o dev_handler -lusb-1.0
 *		  Don't forget to use sudo when running the executable
 * Mac: gcc -I /usr/local/include/libusb-1.0 -L/usr/local/lib/ -lusb-1.0 dev_handler.c -o dev_handler
*/

#ifndef DEV_HANDLER_H
#define DEV_HANDLER_H

#include <libusb.h>
// #include <pthread.h>
#include "../runtime_util/runtime_util.h"
// #include "../shm_wrapper/shm_wrapper.h"
// #include "string.h" // strcmp
#include <stdio.h> // Print
#include <stdlib.h>
#include <stdint.h>
#include <signal.h> // Used to handle SIGTERM, SIGINT, SIGKILL
// #include <time.h>   // For timestamps on device connects/disconnects

#define MAX_PORTS 256

/* A struct defining a lowcar device and the port it is connected to */
// typedef struct usb_dev {
//     libusb_device* dev;
//     dev_id_t* dev_id;
//     // pthread_t* thread;
// } usb_dev_t;

/******************************************
 * STARTING/STOPPING LIBUSB/SHARED MEMORY *
 ******************************************/

/*
 * Initializes data structures, libusb, and shared memory
 */
void init();

/*
 * Frees data structures, stops libusb, and stops shared memory
 */
void stop();

/*
 * Catches Ctrl+C, calls stop(), and exits the program
 */
void sigintHandler(int sig_num);

/*******************************************
 *   FUNCTIONS TO TRACK AND UNTRACK DEVS   *
 *******************************************/
/*
 * Returns whether a device is tracked or not
 * dev: the device to check
 * return: 1 if tracked. 0 otherwise
 */
uint8_t get_tracked_state(libusb_device* dev);

/* Marks a device's tracked value */
void set_tracked_state(libusb_device* dev, uint8_t val);

/*
 * Returns the device that is untracked
 * Assumes that only one device is untracked in LST
 * lst: A list of devices. One of which is untracked
 * len: The length of lst
 * return: The device that is untracked
 */
libusb_device* get_untracked_dev(libusb_device** lst, ssize_t len);

/* Returns the device that is no longer connected in the given list
 * Assumes only one device is disconnected from the list
 * lst: List of devices in which one of them is disconnected
 * len: Length of LST
 */
libusb_device* get_disconnected_dev(libusb_device** lst, ssize_t len);

/* Print the port numbers that are used by tracked devices */
void print_used_ports();

/*******************************************
 *             DEVICE POLLING              *
 *******************************************/

/*
 * Detects when devices are connected and disconnected.
 * On lowcar device connect, spawn a new thread and connect it to shared memory.
 * On lowcar device disconnect, disconnects the device from shared memory.
 */
void poll_connected_devices();

/*******************************************
 *             DEVICE POLLING              *
 *******************************************/
/*
 * Asynchronously sends a PING message to a device to check if it's a lowcar device
 * dev: The device to send a PING to
 */
void ping(libusb_device* dev);

/*
 * The callback function for sending a PING message to a device
 * Prints info about the status of the PING transfer
 */
void ping_callback(struct libusb_transfer* transfer);
#endif
