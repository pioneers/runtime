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

/*
 * Initialize a "global" array of zeros.
 * Acts like a cache to help identify which devices from libusb_get_device_list() are new
 * TRACKED_PORTS[i] is 1 if the device at port i is tracked.
 * Otherwise, it is 0
 */
uint8_t TRACKED_PORTS[MAX_PORTS] = {0};

/*
 * Initialize an array of usb_dev's.
 * Array is added to as lowcar devices are connected
 */
// usb_dev_t CONNECTED_LOWCARS[MAX_DEVICES];

/* Returns whether or not a device is being tracked */
uint8_t get_tracked_state(libusb_device* dev) {
	int port = libusb_get_port_number(dev);
	return TRACKED_PORTS[port];
}

void set_tracked_state(libusb_device* dev, uint8_t val) {
	int port = libusb_get_port_number(dev);
	TRACKED_PORTS[port] = val;
}

/* Returns the device that is untracked in LST */
libusb_device* get_untracked_dev(libusb_device** lst, ssize_t len) {
	for (int i = 0; i < len; i++) {
		if (get_tracked_state(lst[i]) == 0) {
			return lst[i];
		}
	}
	return NULL;
}

/*
 * Returns the device that was disconnected
 * Assumes that only one device was disconnected
 * Try to open every device in LST until LIBUSB_ERROR_NO_DEVICE,
 * which means the device is no longer connected
 */
libusb_device* get_disconnected_dev(libusb_device** lst, ssize_t len) {
	libusb_device_handle* handle = NULL;
	for (int i = 0; i < len; i++) {
		// LIBUSB_ERROR_NO_DEVICE means the device was disconnected
		if (libusb_open(lst[i], &handle) == LIBUSB_ERROR_NO_DEVICE) {
			return lst[i];
		}
		libusb_close(handle);
	}
	return NULL;
}

void print_used_ports() {
	printf("INFO: USED PORTS: ");
	for (int i = 0; i < MAX_PORTS; i++) {
		if (TRACKED_PORTS[i] == 1) {
			printf("%d ", i);
		}
	}
	printf("\n");
}

void poll_connected_devices() {
	// Initialize variables
	libusb_device* dev = NULL;
	libusb_device** tracked;	 // List of connected devices from the previous iteration
	libusb_device** connected;	 // List of connected device in this iteration
	printf("INFO: Getting initial device list\n");
	int num_tracked = libusb_get_device_list(NULL, &tracked);
	int num_connected;

	// Initialize timer
	// time_t current_time;

	// Poll
	printf("INFO: Variables initialized. Polling now.\n");
	while (1) {
		// Check how many are connected
		num_connected = libusb_get_device_list(NULL, &connected);
		// current_time = time(NULL);
		if (num_connected == num_tracked) {
			libusb_free_device_list(connected, 1);
			continue;
		} else {
			if (num_connected > num_tracked) {
				// Mark the new device as tracked and open a thread to for communication
				printf("INFO: NEW DEVICE CONNECTED\n");
				dev = get_untracked_dev(connected, num_connected);
				set_tracked_state(dev, 1);
				/* TODO: Open thread. Thread will send ping packet
				 * If not lowcar device, thread closes.
				 * If lowcar device, connect to shared memory.
				 * Read from and write to device as necessary.
				 * Encountering LIBUSB_ERROR_NO_DEVICE will disconnect from shared memory and close the thread */
			} else if (num_connected < num_tracked) {
				// Mark the disconnected device as untracked
				printf("INFO: DEVICE  DISCONNECTED\n");
				dev = get_disconnected_dev(tracked, num_tracked);
				set_tracked_state(dev, 0);
			}
			print_used_ports();
			libusb_free_device_list(connected, 1);
			// Update tracked devices
			libusb_free_device_list(tracked, 1);
			libusb_get_device_list(NULL, &tracked);
			num_tracked = num_connected;
		}
	}
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
