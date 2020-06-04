/*
 * Handles lowcar device connects/disconnects and acts as the interface between the devices and shared memory
 * Requires third-party library libusb
 * Linux: sudo apt-get install libusb-1.0-0-dev
 *		  gcc -I/usr/include/libusb-1.0 dev_handler.c -o dev_handler -lusb-1.0
 *		  Don't forget to use sudo when running the executable
*/

#ifndef DEV_HANDLER_H
#define DEV_HANDLER_H

#include <libusb.h> // Should work for both Mac and Linux
#include "string.h" // strcmp
#include <stdio.h> // Print
#include <stdlib.h>
#include <stdint.h>
#include <signal.h> // Used to handle SIGTERM, SIGINT, SIGKILL
#include <time.h>   // For timestamps on device connects/disconnects

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
 *   HELPER FUNCTIONS FOR DEVICE POLLING   *
 *******************************************/

/*
 * Returns the index of the libusb device in lst_a not in lst_b
 * lst_a: The list containing an extra libusb device
 * len_a: The length of lst_a. Should be exactly one more than len_b
 * lst_b: The list that should be a subset of lst_a that differs by only one element.
 * lst_b: The length of lst_a. Should be exactly one less than len_b
 * return: The index of the device lst_a that is not in lst_b
 */
int device_diff(libusb_device** lst_a, ssize_t len_a, libusb_device** lst_b, ssize_t len_b);

/*
 * Returns the index of the libusb device in lst that is no longer connected
 * lst: The list of libusb devices to search
 * len: THe length of lst. Len is expected to be exactly one more than the number of devices actually connected
 * return: The index of the device in lst that is not connected
 */
int get_idx_disconnected(libusb_device** lst, ssize_t len);

/*
 * Detects when devices are connected and disconnected.
 * On lowcar device connect, spawn a new thread and connect it to shared memory.
 * On lowcar device disconnect, disconnects the device from shared memory.
 */
void poll_connected_devices();

#endif
