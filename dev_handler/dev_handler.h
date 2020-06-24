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
#include <pthread.h>
#include "../runtime_util/runtime_util.h"
// #include "../shm_wrapper/shm_wrapper.h"
// #include "string.h" // strcmp
#include "message.h"
#include <stdio.h> // Print
#include <stdlib.h>
#include <stdint.h> // ints with specified sizes (uint8_t, uint16_t, etc.)
#include <signal.h> // Used to handle SIGTERM, SIGINT, SIGKILL
#include <time.h>   // For timestamps on HeartBeatRequests
#include <unistd.h> // sleep(int seconds)

// ************************************ CONFIG ************************************* //

// The types of output the DEV_HANDLER can communicate with. FILE_DEV is used for `make fake`
typedef enum output_type { USB_DEV, FILE_DEV };

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

/* The maximum number of milliseconds to wait for a SubscriptionResponse or HeartBeatResponse
 * Waiting for this long will exit all threads for that device (doing cleanup as necessary) */
#define DEVICE_TIMEOUT 4000

/* The number of milliseconds between each HeartBeatRequest sent to the device */
#define HB_REQ_FREQ 2000

#endif
