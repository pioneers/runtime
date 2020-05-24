/*
Handles lowcar device connects/disconnects and acts as the interface between the devices and shared memory
*/

//#include <libusb-1.0/libusb.h> //For Linux
#include <libusb.h> // For Mac
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h> // Used to sleep()
#include <signal.h> // Used to handle SIGTERM, SIGINT, SIGKILL
void clean_up (libusb_device_handle *handle, libusb_context *ctx, libusb_device **lst)
{
	libusb_close(handle);
	libusb_free_device_list(lst, 1);
	libusb_exit(ctx);

}

// Received a signal to stop the process
// Clean up shared memory
void sigintHandler(int sig_num) {
	printf("===Terminating using Ctrl+C===\n");
    fflush(stdout);
	exit(0);

	//shm_stop(<this process>)
}

/* Given a array of connected devices, check if the given device is already connected
*/
int is_connected(dev_id_t** connected_devs, dev_id_t* dev_id) {
    for (int i = 0; i < length of connected devices; i++) {
        for(keys in hashmap){
            if keys is in connected devices{
              break
            }
        }
        removefromHashMap()
    }
    return 0;
}

hashset = {
    (LIMIT_SWITCH, 123), (KOALA_BEAR, 124), ()
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
typedef struct pair {
    int libusb_id;
    dev_id_t dev_id;
} map;


int main ()
{
	// If SIGINT (Ctrl+C) is received, call sigintHandler to clean up
	signal(SIGINT, sigintHandler);

	int ret;
	uint8_t ix;

	libusb_context *ctx;
	libusb_device **lst;
	libusb_device *dev = NULL;
	libusb_device_handle *handle = NULL;
	struct libusb_device_descriptor *desc = (struct libusb_device_descriptor *) malloc (sizeof(struct libusb_device_descriptor) * 1);
	unsigned char *data_buf = (unsigned char *) malloc (sizeof(unsigned char) * 256);

	//initialize the libusb context, check for errors and exit + return on failure
	if ((ret = libusb_init(&ctx)) != 0) {
		printf("libusb_init failed on exit code %d\n", ret);
		libusb_exit(ctx);
		return 1;
	}

	// Print out information on all devices if no error encountered per device.
	int count = 0;
	int delaySec = 5;	// Number of seconds to wait between each iteration
    dev_id_t* connected_devs[MAX_DEVICES]; // Array of structs to keep track of currently connected devices (There are only ever a maximum of 16 devices connected)
	while (count >= 0) {
		printf("===Timestep: %d===\n", count);

		//get the list of all attached USB devices
		ret = libusb_get_device_list(ctx, &lst);
		printf("Number of devices found: %d\n", ret);
		//if we found negative devices, return
		if (ret < 0) {
			return 2;
		}

		int numDevices = ret;
		for (int i = 0; i < numDevices; i++) {
			dev = lst[i];
			//open a handle on the device
			if ((ret = libusb_open(dev, &handle)) != 0) {
				printf("libusb_open failed on exit code %d\n", ret);
				clean_up(handle, ctx, lst);
				continue;
				//return 3;
			}

			//get device descriptor and print out string information
			if ((ret = libusb_get_device_descriptor(dev, desc)) != 0) {
				printf("libusb_get_device_descriptor failed on exit code %d\n", ret);
				clean_up(handle, ctx, lst);
				continue;
				//return 4;
			}

			printf("Device #:%d\n", i);
			printf("	Vendor ID = %d\n", desc->idVendor);
			printf("	Product ID = %d\n", desc->idProduct);

			printf("\n");
		}
		// Wait 5 seconds before
		count++;
		sleep(delaySec);
	}

	/* 1) Continuously poll for newly connected devices (Use libusb)
	 * 2) Add it to a running list of connected devices
	 * 3) Make a thread for it
	 *		shm_init()
	 *		*** If Ctrl+C is hit, call shm_stop() before quitting program
	 		shm_stop(DEV_HANDLER);
	 		shm_stop(EXECUTOR);
	*/

	//free malloced stuff
	free(desc);
	free(data_buf);

	libusb_close(handle); //close the device handle

	libusb_free_device_list(lst, 1); //free the device list while also unreferencing all other detected devices

	libusb_exit(ctx); //deinitialize the libusb context

	return 0;
}
