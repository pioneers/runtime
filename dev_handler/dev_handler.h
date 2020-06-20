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
#include <stdint.h>
#include <signal.h> // Used to handle SIGTERM, SIGINT, SIGKILL
#include <time.h>   // For timestamps on HeartBeatRequests
#include <unistd.h> // sleep(int seconds)

/*
 * A USB device can have one or more interfaces.
 * This defines which interface we want the DEV_HANDLER to claim
 * It is an error if this is larger than the number of interfaces a device has.
 */
#define INTERFACE_IDX 1

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
    pthread_t sender;
    pthread_t receiver;
    pthread_t relayer;
    uint8_t send_endpoint;
    uint8_t receive_endpoint;
    int shm_dev_idx;        // The unique index assigned to the device by shm_wrapper for shared memory operations on device_connect()
    uint8_t start;		    // set by relayer: A flag to tell reader and receiver to start work when set to 1
    dev_id_t dev_id;        // set by relayer once SubscriptionResponse is received
    uint64_t sent_hb_req;	// set by sender: The TIME that a HeartBeatRequest was sent. Relay should expect a hb response
    uint8_t got_hb_req;	// set by receiver: A flag to tell sender to send a HeartBeatResponse
} msg_relay_t;

/* The maximum number of milliseconds to wait for a SubscriptionResponse or HeartBeatResponse
 * Waiting for this long will exit all threads for that device (doing cleanup as necessary) */
#define DEVICE_TIMEOUT 1000

/* The number of milliseconds between each HeartBeatRequest sent to the device */
#define HB_REQ_FREQ 500

/*
 * Opens threads for communication with a device
 * Three threads will be opened:
 *  relayer: Verifies device is lowcar and cancels threads when device disconnects/timesout
 *  sender: Sends changed data in shared memory and periodically sends HeartBeatRequests
 *  receiver: Receives data from the lowcar device and signals sender thread about HeartBeatRequests
 *
 * dev: The libusb_device to communicate with
 */
void communicate(libusb_device* dev);

/*
 * Sends a Ping to the device and waits for a SubscriptionResponse
 * If the SubscriptionResponse takes too long, close the device and exit all threads
 * Connects the device to shared memory and signals the sender and receiver to start
 * Continuously checks if the device disconnected or HeartBeatRequest was sent without a response
 *      If so, it disconnects the device from shared memory, closes the device, and frees memory
 * RELAY_CAST is to be casted as msg_relay_t
 */
void* relayer(void* relay);

/* Called by relayer to cancel threads and free RELAY */
void relay_clean_up(msg_relay_t* relay);

/*
 * Continuously reads from shared memory to serialize the necessary information
 * to send to the device.
 * Sets relay->sent_hb_req when a HeartBeatRequest is sent
 * Sends a HeartBeatResponse when relay->got_hb_req is set
 * RELAY_CAST is to be casted as msg_relay_t
 */
void* sender(void* relay);

/*
 * Continuously attempts to parse incoming data over serial and send to shared memory
 * Sets relay->got_hb_req upon receiving a HeartBeatRequest
 * RELAY_CAST is to be casted as msg_relay_t
 */
void* receiver(void* relay);

/*******************************************
 *            DEVICE MESSAGES              *
 *******************************************/

/*
 * Serializes and sends a message to the specified endpoint
 * msg: The message to be sent, constructed from the functions below
 * handle: A libusb device handle obtained by using libusb_open on the device
 * endpoint: The usb endpoint address that the message should be sent to
 * transferred: Output location for number of bytes actually sent
 * return: The return value of libusb_bulk_transfer
 *      http://libusb.sourceforge.net/api-1.0/group__libusb__syncio.html#gab8ae853ab492c22d707241dc26c8a805
 */
int send_message(message_t* msg, libusb_device_handle* handle, uint8_t endpoint, int* transferred);

/*
 * Attempts to read a message from the specified endpoint
 * msg: Output empty message to be populated with the read message if successful
 * handle: A libusb device handle obtained by using libusb_open on the device
 * endpoint: The usb endpoint address that the message should be read from
 * return: 0 if a full message was successfully read and put into MSG
 *         1 if no message was found
 *         2 if a broken message was received
 *         3 if a message was received but with an incorrect checksum
 */
int receive_message(message_t* msg, libusb_device_handle* handle, uint8_t endpoint);

/*
 * Synchronously sends a PING message to a device to check if it's a lowcar device
 * relay: Struct holding device, handle, and endpoint fields
 * return: 0 upon successfully receiving a SubscriptionResponse and setting relay->dev_id
 *      1 if Ping message couldn't be serialized
 *      2 if Ping message couldn't be sent or SubscriptionResponse wasn't received
 *      3 if received message has incorrect checksum
 *      4 if received message is not a SubscriptionResponse
 */
int ping(msg_relay_t* relay);

/*******************************************
 *                 UTILITY                 *
 *******************************************/
/* Returns the number of milliseconds since the Unix Epoch */
int64_t millis();
/* Prints the byte array DATA of length LEN in hex */
void print_bytes(uint8_t* data, int len);
#endif
