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
	int ret;
	// Init libusb
	if ((ret = libusb_init(NULL)) != 0) {
		printf("libusb_init failed on exit code %d\n", ret);
		libusb_exit(NULL);
		return;
	}
}

// Free memory and safely stop connections
void stop() {
	libusb_exit(NULL);
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
 * Return the index of the device in lst_a that is NOT in lst_b
 * Return -1 if len_a is not exactly one more than len_b
 */
int device_diff(libusb_device** lst_a, ssize_t len_a, libusb_device** lst_b, ssize_t len_b) {
	// Return -1 if len_a is not exactly one more than len_b
	if (len_a != len_b + 1) {
		return -1;
	}

	// Initialize variables
    int ret;
	uint8_t is_diff = 1;
	libusb_device_handle* handle = NULL;
	struct libusb_device_descriptor* desc = malloc (sizeof(struct libusb_device_descriptor));
	unsigned char* serial_number_a = malloc(256 * sizeof(unsigned char));
	unsigned char* serial_number_b = malloc(256 * sizeof(unsigned char));
	// Loop
	for (int i = 0; i < len_a; i++) {
		// Open a handle on the device
		if ((ret = libusb_open(lst_a[i], &handle)) != 0) {
			printf("ERROR: libusb_open failed on exit code %s\n", libusb_error_name(ret));
			continue;
		}
		// Get device descriptor
		if ((ret = libusb_get_device_descriptor(lst_a[i], desc)) != 0) {
			printf("ERROR: libusb_get_device_descriptor failed on exit code %s\n", libusb_error_name(ret));
			continue;
		}
		// Get serial number of dev_a
		libusb_get_string_descriptor_ascii(handle, desc->iSerialNumber, serial_number_a, 256);
		libusb_close(handle);
		// Check if it's in lst_b
		is_diff = 1;
		for (int j = 0; j < len_b; j++) {
			// Open a handle on the device
			if ((ret = libusb_open(lst_a[i], &handle)) != 0) {
				printf("ERROR: libusb_open failed on exit code %s\n", libusb_error_name(ret));
				continue;
			}
			// Get device descriptor
			if ((ret = libusb_get_device_descriptor(lst_a[i], desc)) != 0) {
				printf("ERROR: libusb_get_device_descriptor failed on exit code %s\n", libusb_error_name(ret));
				continue;
			}
			// Get serial number of dev b
			libusb_get_string_descriptor_ascii(handle, desc->iSerialNumber, serial_number_b, 256);
			libusb_close(handle);
			// Compare
			if (strcmp(serial_number_a, serial_number_b) == 0) {
				// dev_a is in lst_b. Move onto the next device in lst_a
				is_diff = 0;
				break;
			}
		}
		if (is_diff) {
			free(desc);
			free(serial_number_a);
			free(serial_number_b);
			return i;
		}
	}
	// Could not identify the extra device for some reason
	free(desc);
	free(serial_number_a);
	free(serial_number_b);
	return -2;
}

/*
 * Returns the index of the device that was disconnected
 * Assumes that only one device was disconnected
 */
int get_idx_disconnected(libusb_device** lst, ssize_t len) {
	libusb_device_handle* handle = NULL;
	for (int i = 0; i < len; i++) {
		// LIBUSB_ERROR_NO_DEVICE means the device was disconnected
		if (libusb_open(lst[i], &handle) == LIBUSB_ERROR_NO_DEVICE) {
			return i;
		}
		libusb_close(handle);
	}
}

void poll_connected_devices() {
	// Initialize variables
	int ret; // Variable to hold return values for success/failure
	libusb_device_handle* handle_tracked = NULL;
	libusb_device_handle* handle_connected = NULL;

	// Initialize list of connected devices
	libusb_device** tracked;
	libusb_device** connected;
	printf("INFO: Getting initial device list\n");
	int num_tracked = libusb_get_device_list(NULL, &tracked);
	int num_connected;

	// Initialize timer
	time_t current_time;

	// Poll
	printf("INFO: Variables initialized. Polling now.\n");
	while (1) {
		// Check how many are connected
		num_connected = libusb_get_device_list(NULL, &connected);
		if (num_connected > num_tracked) {
			current_time = time(NULL);
			printf("INFO: %s. Timestamp: %s", "NEW DEVICE CONNECTED", ctime(&current_time));
			// Find out which connected device isn't tracked.
			ret = device_diff(connected, num_connected, tracked, num_tracked);
			// Open device
			// libusb_open(connected[ret], &handle);
			// TODO: Open new thread
			// Update tracked devices
			libusb_free_device_list(tracked, 1);
			libusb_get_device_list(NULL, &tracked);
			num_tracked = num_connected;
		} else if (num_connected < num_tracked) {
			current_time = time(NULL);
			printf("INFO: %s. Timestamp: %s", "DEVICE  DISCONNECTED", ctime(&current_time));
			// Find out which tracked device was disconnected
			ret = get_idx_disconnected(tracked, num_tracked);
			// TODO: Close thread
			// TODO: Close device
			// Update tracked devices
			libusb_free_device_list(tracked, 1);
			libusb_get_device_list(NULL, &tracked);
			num_tracked = num_connected;
		}
		libusb_free_device_list(connected, 1);
	}
}

int main() {
	// If SIGINT (Ctrl+C) is received, call sigintHandler to clean up
	signal(SIGINT, sigintHandler);

	/* 1) Continuously poll for newly connected devices (Use libusb)
	 * 2) Add it to a running list of connected devices
	 * 3) Make a thread for it
	 *		send a Ping packet and wait for a SubscriptionResponse to verify that it's a lowcar device
	 *		If it's a lowcar device, connect it to shared memory and set up data structures to poll for incoming/outgoing messages
     *	If Ctrl+C is hit, call shm_stop() before quitting program
 		shm_stop(DEV_HANDLER);
 		shm_stop(EXECUTOR);
	 */
	init();

	poll_connected_devices();

	stop();
	return 0;
}
