/*
 * Handles lowcar device connects/disconnects and acts as the interface between the devices and shared memory
 * Requires third-party library libusb
 * Linux: sudo apt-get install libusb-1.0-0-dev
 *		  gcc -I/usr/include/libusb-1.0 dev_handler.c -o dev_handler -lusb-1.0
*/

#include <libusb.h> // Should work for both Mac and Linux
#include "string.h" // strcmp
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
// #include <unistd.h> // Used to sleep()
#include <signal.h> // Used to handle SIGTERM, SIGINT, SIGKILL
#include <time.h>   // For timestamps on device connects/disconnects

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
void clean_up ()
{
	libusb_exit(NULL);
}

// Called when attempting to terminate program with ctrl+C
void sigintHandler(int sig_num) {
	printf("\nINFO: Ctrl+C pressed. Safely terminating program\n");
    fflush(stdout);
	exit(0);

	//shm_stop(<this process>)
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
			printf("INFO: %s. Timestamp: %s\n", "NEW DEVICE CONNECTED", ctime(&current_time));
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
			printf("INFO: %s. Timestamp: %s\n", "A DEVICE HAS DISCONNECTED", ctime(&current_time));
			// Find out which tracked device was disconnected
			ret = device_diff(tracked, num_tracked, connected, num_connected);
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

// Running while loop: Check whenever a device is connected
// If a device is connected,
//   From libusb, we know it's vendorid, productid, serialnumber, etc
//   Ask for its type and UID (hibike_process.py, subscription request))
//   Add to as hashmap: libusbID --> type, uid
//      Ex: (123) --> (KOALA_BEAR, 0x13)
//      Now we know libusb associates vendorID 123 with a KoalaBear
//   shared memory device_connect(type, UID)
//

// Maps a libusb vendorID to a device
// typedef struct pair {
//     int libusb_id;
//     dev_id_t dev_id;
// } map;

int main() {
	// If SIGINT (Ctrl+C) is received, call sigintHandler to clean up
	signal(SIGINT, sigintHandler);

	/* 1) Continuously poll for newly connected devices (Use libusb)
	 * 2) Add it to a running list of connected devices
	 * 3) Make a thread for it
	 *		shm_init()
	 *		*** If Ctrl+C is hit, call shm_stop() before quitting program
	 		shm_stop(DEV_HANDLER);
	 		shm_stop(EXECUTOR);
	*/
	init();

	poll_connected_devices();

	clean_up(); //deinitialize the libusb context
	return 0;
}
