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
#include "message.h"
#include <stdio.h> // Print
#include <stdlib.h>
#include <stdint.h>
#include <signal.h> // Used to handle SIGTERM, SIGINT, SIGKILL
// #include <time.h>   // For timestamps on device connects/disconnects

#define MAX_PORTS 256

/*
 * A struct defining a device's information for the lifetime of its connection
 * Fields can be easily retrieved without requiring libusb_open()
 */
typedef struct usb_id {
    uint16_t vendor_id;    // Obtained from struct libusb_device_descriptor field from libusb_get_device_descriptor()
    uint16_t product_id;   // Same as above
    uint8_t dev_addr;      // Obtained from libusb_get_device_address(libusb_device*)
} usb_id_t;

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
 * Allocates memory for an array of usb_id_t and populates it with data from the provided list
 * lst: Array of libusb_devices to have their vendor_id, product_id, and device address to be put in the result
 * len: The length of LST
 * return: Array of usb_id_t pointers of length LEN. Must be freed with free_tracked_devices()
 */
usb_id_t** alloc_tracked_devices(libusb_device** lst, int len);

/*
 * Frees the given list of usb_id_t pointers
 */
void free_tracked_devices(usb_id_t** lst, int len);

/*
 * Returns a pointer to the libusb_device in CONNECTED but not in TRACKED
 * connected: List of libusb_device*
 * num_connected: Length of CONNECTED
 * tracked: List of usb_id_t* that holds info about every device in CONNECTED except for one
 * num_tracked: Length of TRACKED. Expected to be exactly one less than connected
 * return: Pointer to libusb_device
 */
libusb_device* get_new_device(libusb_device** connected, int num_connected, usb_id_t** tracked, int num_tracked);

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
 *           READ/WRITE THREADS            *
 *******************************************/
/* Struct shared between the three threads so they can communicate with each other
 * relayer thread acts as the "control center" and connects/disconnects to shared memory
 *  relayer thread also frees memory when device is disconnected
 */
typedef struct msg_relay {
    libusb_device* dev;
    libusb_device_handle* handle;
    pthread_t* sender;
    pthread_t* receiver;
    dev_id_t* dev_id;       // set by relayer once SubscriptionResponse is received
    uint8_t start;		    // set by relayer; a flag to tell reader and receiver to start work
    uint8_t sent_hb_req;	// set by write: relay should expect a hb response
    uint8_t got_hb_req;	    // set by read: write should send a hb response
} msg_relay_t;

/* The max number of seconds to wait for a SubscriptionResponse or HeartBeatRequest
 * Waiting for this long will exit all threads (doing cleanup as necessary) */
#define DEVICE_TIMEOUT 5;

/*
 * Sends a Ping to the device and waits for a SubscriptionResponse
 * If the SubscriptionResponse takes too long, close the device and exit all threads
 * Connects the device to shared memory and signals the sender and receiver to start
 * Continuously checks if the device disconnected or HeartBeatRequest was sent without a response
 *      If so, it disconnects the device from shared memory, closes the device, and frees memory
 */
void relayer(msg_relay_t* relay);

/*
 * Continuously reads from shared memory to serialize the necessary information
 * to send to the device.
 * Sets relay->sent_hb_req when a HeartBeatRequest is sent
 * Sends a HeartBeatResponse when relay->got_hb_req is set
 */
void sender(msg_relay_t* relay);

/*
 * Continuously attempts to parse incoming data over serial and send to shared memory
 * Sets relay->got_hb_req upon receiving a HeartBeatRequest
 */
void receiver(msg_relay_t* relay);

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
