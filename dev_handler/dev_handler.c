/*
 * Handles lowcar device connects/disconnects and acts as the interface between the devices and shared memory
 * Requires third-party library libusb
 * Linux: sudo apt-get install libusb-1.0-0-dev
 *		  gcc -I/usr/include/libusb-1.0 dev_handler.c -o dev_handler -lusb-1.0
 *		  Don't forget to use sudo when running the executable
*/

#include "dev_handler.h"

// Initialize data structures / connections
void init() {
	// Init shared memory
	// shm_init(DEV_HANDLER);
	// Init libusb
	int ret;
	if ((ret = libusb_init(NULL)) != 0) {
		printf("libusb_init failed on exit code %d\n", ret);
		libusb_exit(NULL);
		return;
	}
}

// Free memory and safely stop connections
void stop() {
	libusb_exit(NULL);
	// shm_stop(DEV_HANDLER);
}

// Called when attempting to terminate program with ctrl+C
void sigintHandler(int sig_num) {
	printf("\nINFO: Ctrl+C pressed. Safely terminating program\n");
    fflush(stdout);

	// TODO: For each tracked lowcar device, disconnect from shared memory

	stop();
	exit(0);
}

usb_id_t** alloc_tracked_devices(libusb_device** lst, int len) {
	struct libusb_device_descriptor desc;
	usb_id_t** result = malloc(len * sizeof(usb_id_t*));
	printf("INFO: There are now %d tracked devices.\n", len);
	for (int i = 0; i < len; i++) {
		result[i] = malloc(sizeof(usb_id_t));
		libusb_device* dev = lst[i];
		libusb_get_device_descriptor(dev, &desc);
		result[i]->vendor_id = desc.idVendor;
		result[i]->product_id = desc.idProduct;
		result[i]->dev_addr = libusb_get_device_address(dev);
	}
	return result;
}

void free_tracked_devices(usb_id_t** lst, int len) {
	for (int i = 0; i < len; i++) {
		free(lst[i]);
	}
	free(lst);
}

libusb_device* get_new_device(libusb_device** connected, int num_connected, usb_id_t** tracked, int num_tracked) {
	libusb_device* dev;
	struct libusb_device_descriptor desc;
	// Loop through each connected device
	for (int i = 0; i < num_connected; i++) {
		dev = connected[i];
		libusb_get_device_descriptor(dev, &desc);
		uint16_t vendor_id = desc.idVendor;
		uint16_t product_id = desc.idProduct;
		uint8_t dev_addr = libusb_get_device_address(dev);
		// If it's not in tracked, it's a new device
		uint8_t in_tracked = 0;
		for (int j = 0; j < num_tracked; j++) {
			if (vendor_id == tracked[i]->vendor_id &&
				product_id == tracked[i]->product_id &&
				dev_addr == tracked[i]->dev_addr) {
					in_tracked = 1;
					break;
				}
		}
		if (in_tracked == 0) {
			libusb_get_device_descriptor(dev, &desc);
			printf("INFO:    Vendor:Product: %u:%u\n", desc.idVendor, desc.idProduct);
			return connected[i];
		}
	}
	return NULL;
}

void poll_connected_devices() {
	// Initialize variables
	libusb_device* dev = NULL;
	libusb_device** connected;	 										// List of connected device in this iteration
	printf("INFO: Getting initial device list\n");
	int num_tracked = libusb_get_device_list(NULL, &connected);			// Number of connected devices in previous iteration
	usb_id_t** tracked = alloc_tracked_devices(connected, num_tracked);	// List of connected deevices in previous iteration
	libusb_free_device_list(connected, 1);
	int num_connected;													// Number of connected devices in this iteration

	// Initialize timer
	// time_t current_time;

	// Poll
	printf("INFO: Variables initialized. Polling now.\n");
	while (1) {
		// Check how many are connected
		num_connected = libusb_get_device_list(NULL, &connected);
		if (num_connected == num_tracked) {
			libusb_free_device_list(connected, 1);
			continue;
		} else {
			if (num_connected > num_tracked) {
				// Mark the new device as tracked and open a thread to for communication
				printf("INFO: NEW DEVICE CONNECTED\n");
				dev = get_new_device(connected, num_connected, tracked, num_tracked);
				/* TODO: Open thread. Thread will send ping packet
				 * If not lowcar device, thread closes.
				 * If lowcar device, connect to shared memory.
				 * Read from and write to device as necessary.
				 * Encountering LIBUSB_ERROR_NO_DEVICE will disconnect from shared memory and close the thread */
			} else if (num_connected < num_tracked) {
				printf("INFO: DEVICE  DISCONNECTED\n");
				// TODO: ?
			}
			// Free connected list
			libusb_free_device_list(connected, 1);
			// Update tracked devices
			free_tracked_devices(tracked, num_tracked);
			tracked = alloc_tracked_devices(connected, num_connected);
			num_tracked = num_connected;
		}
	}
}

/*
 * Send a PING message to the device and wait for a SubscriptionResponse
 */
void ping(libusb_device* dev) {
	// Make a PING message to be sent over serial
	message_t* ping = make_ping();
	int len = calc_max_cobs_msg_length(ping);
	uint8_t* data = malloc(len * sizeof(uint8_t));
	int ret = message_to_bytes(ping, data, len);
	destroy_message(ping);
	if (ret != 0) {
		printf("ERROR: Couldn't serialize ping message in investigate_port()\n");
		return;
	}
	// Open the device
	libusb_device_handle* handle;
	libusb_open(dev, &handle);

	// Allocate a libusb_transfer: http://libusb.sourceforge.net/api-1.0/group__libusb__asyncio.html#ga13cc69ea40c702181c430c950121c000
	// libusb_transfer: http://libusb.sourceforge.net/api-1.0/structlibusb__transfer.html
	struct libusb_transfer* ping_transfer = libusb_alloc_transfer(0); // API says use 0 for libusb_bulk_transfer
	libusb_fill_bulk_transfer(ping_transfer, handle, /* TODO: ENDPOINT */ 0, \
							data, len, /*CALLBACK*/ (libusb_transfer_cb_fn) ping_callback, /*USER_DATA*/ NULL, /*TIMEOUT (ms)*/ 5000);

	// Send it over
	libusb_submit_transfer(ping_transfer);

	libusb_close(handle);
	free(data);
	libusb_free_transfer(ping_transfer);
}

/* Callback function after sending a PING message to a device */
void ping_callback(struct libusb_transfer* transfer) {
	if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
		printf("FATAL: PING message transfer occurred an error with code %d\n", transfer->status);
		return;
	} else if (transfer->actual_length != transfer->length) {
		printf("FATAL: PING message transfer wasn't sent completely\n");
		return;
	}
	printf("INFO: Successfuly sent PING message to device\n");
}
/* 1) Continuously poll for newly connected devices (Use libusb)
 * 2) Add it to a running list of connected devices
 * 3) Make a thread for it
 *		send a Ping packet and wait for a SubscriptionResponse to verify that it's a lowcar device
 *		If it's a lowcar device, connect it to shared memory and set up data structures to poll for incoming/outgoing messages
 *	If Ctrl+C is hit, call shm_stop() before quitting program
	shm_stop(DEV_HANDLER);
	shm_stop(EXECUTOR);
 */
int main() {
	// If SIGINT (Ctrl+C) is received, call sigintHandler to clean up
	signal(SIGINT, sigintHandler);
	init();

	poll_connected_devices();
	return 0;
}
