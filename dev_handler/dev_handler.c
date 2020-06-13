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
	printf("\nINFO: Ctrl+C pressed. Safely terminating program\n");
    fflush(stdout);
	// Exit libusb
	libusb_exit(NULL);
	// TODO: For each tracked lowcar device, disconnect from shared memory
	// TODO: Stop shared memory
	// shm_stop(DEV_HANDLER);
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
				// Communicate with the new device
				printf("INFO: NEW DEVICE CONNECTED\n");
				dev = get_new_device(connected, num_connected, tracked, num_tracked);
				communicate(dev);
			} else if (num_connected < num_tracked) {
				printf("INFO: DEVICE  DISCONNECTED\n");
				// TODO: ?
			}
			// Update tracked devices
			free_tracked_devices(tracked, num_tracked);
			tracked = alloc_tracked_devices(connected, num_connected);
			num_tracked = num_connected;
			// Free connected list
			libusb_free_device_list(connected, 1);
		}
	}
}

/*******************************************
 *           READ/WRITE THREADS            *
 *******************************************/
/*
* Opens threads for communication with a device
* Threads will cleanup after themselves when they exit
* dev: The libusb_device to communicate with
*/
void communicate(libusb_device* dev) {
	msg_relay_t* relay = malloc(sizeof(msg_relay_t));
	relay->dev = dev;
	// Open the device
	int ret = libusb_open(dev, &relay->handle);
	if (ret != 0) {
		printf("ERROR: libusb_open in communicate() with error code %s\n", libusb_error_name(ret));
	}
	// Initialize relay->start as 0, indicating sender and receiver to not start work yet
	relay->start = 0;
	// Open threads for sender, receiver, and relayer
	pthread_create(&relay->sender, NULL, sender, relay);
	pthread_create(&relay->receiver, NULL, receiver, relay);
	pthread_create(&relay->relayer, NULL, relayer, relay);
}

/*
* Sends a Ping to the device and waits for a SubscriptionResponse
* If the SubscriptionResponse takes too long, close the device and exit all threads
* Connects the device to shared memory and signals the sender and receiver to start
* Continuously checks if the device disconnected or HeartBeatRequest was sent without a response
*      If so, it disconnects the device from shared memory, closes the device, and frees memory
*/
void* relayer(void* relay_cast) {
	msg_relay_t* relay = relay_cast;
	int ret;
	// Get the device's active config to find the number of interfaces it has
	struct libusb_config_descriptor* config;
	ret = libusb_get_active_config_descriptor(relay->dev, &config);
	if (ret != 0) {
		printf("ERROR: Couldn't get active config descriptor with error code %s\n", libusb_error_name(ret));
		return NULL;
	}
	printf("INFO: Device has %d interfaces\n", config->bNumInterfaces);

	// Tell libusb to automatically detach the kernel driver on claimed interfaces then reattach them when releasing the interface
	ret = libusb_set_auto_detach_kernel_driver(relay->handle, 1);
	if (ret == LIBUSB_ERROR_NOT_SUPPORTED) {
		printf("ERROR: Can't set auto-detach kernel driver on device handle\n");
		return NULL;
	}

	// Claim an interface on the device's handle, telling OS we want to do I/O on the device
	ret = libusb_claim_interface(relay->handle, 0); // TODO: Getting the first one. Change later?
	if (ret != 0) {
		printf("ERROR: Couldn't claim interface with error code %s\n", libusb_error_name(ret));
		return NULL;
	}
	printf("INFO: Successfully claimed 0th interface!\n");
	const struct libusb_interface interface = config->interface[0];
	printf("INFO: 0th interface has %d settings\n", interface.num_altsetting);
	const struct libusb_interface_descriptor interface_desc = interface.altsetting[0]; // TODO: Getting the first one. Change later?

	// Get the endpoints for sending and receiving
	printf("INFO: 0th setting of 0th interface has %d endpoints\n", interface_desc.bNumEndpoints);
	uint8_t found_bulk_in = 0;
	uint8_t found_bulk_out = 0;
	struct libusb_endpoint_descriptor endpoint;
	for (int i = 0; i < interface_desc.bNumEndpoints; i++) { // Iterate through the endpoints until we get both bulk endpoint in and bulk endpoint out
		endpoint = interface_desc.endpoint[i];
		// Bits 0 and 1 of endpoint.bmAttributes detail the transfer type. We want bulk
		if ((3 & endpoint.bmAttributes) != LIBUSB_TRANSFER_TYPE_BULK) {
			continue;
		}
		// Bit 7 of endpoint.bEndpointAddress indicates the direction
		if (!found_bulk_in && (((1 << 7) & endpoint.bEndpointAddress) == LIBUSB_ENDPOINT_IN)) { // TODO: Double check this logic
			// LIBUSB_ENDPOINT_IN: Transfer from device to dev_handler
			relay->receive_endpoint = endpoint.bEndpointAddress;
			found_bulk_in = 1;
		} else if (!found_bulk_out && (((1 << 7) & endpoint.bEndpointAddress) == LIBUSB_ENDPOINT_OUT)) {
			// LIBUSB_ENDPOINT_OUT: Transfer from dev_handler to device
			relay->send_endpoint = endpoint.bEndpointAddress;
			found_bulk_out = 1;
		} else if (found_bulk_in && found_bulk_out) {
			break;
		}
	}
	if (!found_bulk_in) {
		printf("FATAL: Couldn't get endpoint for receiver\n");
	}
	if (!found_bulk_out) {
		printf("FATAL: Couldn't get endpoint for sender\n");
	}
	if (!found_bulk_in || !found_bulk_out) {
		libusb_release_interface(relay->handle, 0);
		return NULL;
	}
	printf("INFO: Endpoints were successfully identified!\n");
	printf("INFO:     Send (OUT) Endpoint: 0x%X\n", relay->send_endpoint);
	printf("INFO:     Receive (IN) Endpoint: 0x%X\n", relay->receive_endpoint);

	// Send a Ping and wait for a SubscriptionResponse
	printf("INFO: Relayer will send a Ping to the device\n");
	ret = ping(relay);
	if (ret != 0) {
		relay_clean_up(relay);
		return NULL;
	}
	// TODO: Connect the lowcar device to shared memory
	printf("INFO: Relayer will connect the device to shared memory\n");

	// Signal the sender and receiver to start work
	printf("INFO: Relayer broadcasting to Sender and Receiver to start work\n");
	relay->start = 1;

	// If the device disconnects or a HeartBeatRequest takes too long to be resolved, clean up
	printf("INFO: Relayer will begin to monitor the device\n");
	struct libusb_device_descriptor desc;
	while (1) {
		// Getting the device descriptor if the device is disconnected should give an error
		ret = libusb_get_device_descriptor(relay->dev, &desc);
		if ((ret != 0) || (relay->sent_hb_req >= DEVICE_TIMEOUT)) {
			// TODO: Disconnect from shared memory
			printf("INFO: Device disconnected or timed out!\n");
			relay_clean_up(relay);
			return NULL;
		}
	}
}

void relay_clean_up(msg_relay_t* relay) {
	libusb_release_interface(relay->handle, 0);
	libusb_close(relay->handle);
	pthread_cancel(relay->sender);
	pthread_cancel(relay->receiver);
	free(relay);
	pthread_cancel(pthread_self());
}

/*
* Continuously reads from shared memory to serialize the necessary information
* to send to the device.
* Sets relay->sent_hb_req when a HeartBeatRequest is sent
* Sends a HeartBeatResponse when relay->got_hb_req is set
*/
void* sender(void* relay_cast) {
	msg_relay_t* relay = relay_cast;
	// Sleep until relay->start == 1, which is true when relayer receives a SubscriptionResponse
	while (1) {
		if (relay->start == 1) {
			break;
		}
		pthread_testcancel(); // Cancellation point. Relayer may cancel this thread if device timesout
	}

	/* Do work! Continuously read shared memory param bitmap. If it changes,
	 * Call device_read to get the changed parameters and send them to the device
	 * Also, send HeartBeatRequest at a regular interval, HB_REQ_FREQ.
	 * Finally, send HeartBeatResponse when receiver gets a HeartBeatRequest */
	uint32_t* last_pmap;
	uint32_t* pmap_now;
	while (1) {
		// If parameter bitmap changed, get the changed parameters
		// get_param_bitmap(pmap_now);
		// TODO: Compare them. If diff, device_read and send data to device

		if (relay->got_hb_req) {
			// TODO: Send HeartBeatResponse
			relay->got_hb_req = 0; // Mark as resolved
		}
		pthread_testcancel(); // Cancellation point
	}
	return NULL;
}

/*
* Continuously attempts to parse incoming data over serial and send to shared memory
* Sets relay->got_hb_req upon receiving a HeartBeatRequest
*/
void* receiver(void* relay_cast) {
	msg_relay_t* relay = relay_cast;
	// Sleep until relay->start == 1, which is true when relayer receives a SubscriptionResponse
	while (1) {
		if (relay->start == 1) {
			break;
		}
		pthread_testcancel(); // Cancellation point. Relayer may cancel this thread if device timesout
	}

	/* Do work! Continuously read from the device until a complete message can be parsed.
	 * If it's a HeartBeatRequest, signal sender to reply with a HeartBeatResponse
	 * If it's a HeartBeatResponse, mark the current HeartBeatRequest as resolved
	 * If it's device data, write it to shared memory
	 */
	while (1) {
		// TODO: Read data from device
		// TODO: Try to parse
		// TODO: If HeartBeatRequest, relay->got_hb_req = 1
		// TODO: If HeartBeatResponse, relay->sent_hb_req = 0;
		// TODO: If DeviceData, send to shared memory
		pthread_testcancel(); // Cancellation point
	}
	return NULL;
}

/*
 * Send a PING message to the device and wait for a SubscriptionResponse
 */
int ping(msg_relay_t* relay) {
	// Make a PING message to be sent over serial
	message_t* ping = make_ping();
	int len = calc_max_cobs_msg_length(ping);
	uint8_t* data = malloc(len * sizeof(uint8_t));
	int ret = message_to_bytes(ping, data, len);
	destroy_message(ping);
	// ret holds the length of data (the length of the serialized Ping message)
	if (ret <= 0) {
		printf("ERROR: Couldn't serialize ping message in ping()\n");
		free(data);
		return 1;
	}

	// Bulk transfer the Ping message to the device
	printf("INFO: Ping message serialized and ready to transfer\n");
	int* transferred; // The number of bytes actually sent
	ret = libusb_bulk_transfer(relay->handle, relay->send_endpoint, data, ret, transferred, DEVICE_TIMEOUT);
	if (ret != 0) {
		printf("ERROR: Couldn't bulk transfer Ping with error code %s\n", libusb_error_name(ret));
		free(data);
		return 2;
	}
	free(data);
	data = NULL;
	printf("INFO: Ping message successfully sent!\n");

	// Try to read a SubscriptionResponse, which we expect from a lowcar device that receives a Ping
	data = malloc(32); // It shouldn't be more than 32 bytes. ~20 + cobs
	printf("INFO: Listening for SubscriptionResponse\n");
	ret = libusb_bulk_transfer(relay->handle, relay->receive_endpoint, data, 32, transferred, DEVICE_TIMEOUT);
	if (ret != 0) {
		printf("ERROR: Couldn't bulk transfer SubscriptionResponse with error code %s\n", libusb_error_name(ret));
		return 2;
	}
	// Parse the received message to verify it's a SubscriptionResponse
	printf("INFO: Data received and will be parsed!\n");
	message_t* sub_response = malloc(sizeof(message_t));
	ret = parse_message(data, sub_response);
	free(data); // We don't need the encoded message anymore
	if (ret != 0) {
		printf("FATAL: Received data with incorrect checksum\n");
		free(sub_response);
		return 3;
	} else if (sub_response->message_id != SubscriptionResponse) {
		printf("FATAL: Message is not a SubscriptionResponse\n");
		free(sub_response);
		return 4;
	}
	// We have a SubscriptionResponse!
	printf("INFO: SubscriptionResponse received!\n");
	relay->dev_id.type = ((uint16_t*)(&sub_response->payload[6]))[0];	// device type is the 16-bits starting at the 6th byte
	relay->dev_id.year = sub_response->payload[8];						// device year is the 8th byte
	relay->dev_id.uid  = ((uint64_t*)(&sub_response->payload[9]))[0];	// device uid is the 64-bits starting at the 9th byte
	return 0;
}

int main() {
	// If SIGINT (Ctrl+C) is received, call sigintHandler to clean up
	signal(SIGINT, stop);
	init();
	// Set libusb to log debug messages
	// if (libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING) != LIBUSB_SUCCESS) {
	// 	printf("ERROR: libusb log level could not be set\n");
	// }
	// Start polling
	poll_connected_devices();
	return 0;
}
