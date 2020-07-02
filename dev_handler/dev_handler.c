#include "dev_handler.h"

// ************************************ PRIVATE TYPEDEFS ****************************************** //
// The baud rate set on the Arduino
#define BAUD_RATE 115200

/* Struct shared between the three threads so they can communicate with each other
 * relayer thread acts as the "control center" and connects/disconnects to shared memory
 *  relayer thread also frees memory when device is disconnected
 */
typedef struct msg_relay {
    pthread_t sender;
    pthread_t receiver;
    pthread_t relayer;
    uint8_t port_num;       // where port is "/dev/ttyACM<port_num>/"
    FILE* read_file;
    FILE* write_file;
    int file_descriptor;    // Obtained from using open() and used to close()
    int shm_dev_idx;        // The unique index assigned to the device by shm_wrapper for shared memory operations on device_connect()
    uint8_t start;		    // set by relayer: A flag to tell reader and receiver to start work when set to 1
    dev_id_t dev_id;        // set by relayer once SubscriptionResponse is received
    int8_t got_hb_req;	    // set by receiver: A flag to tell sender to send a HeartBeatResponse
    uint64_t last_sent_hbreq_time;      // set by sender: The last timestamp of a sent HeartBeatRequest. Used to detect timeout
    uint64_t last_received_hbresp_time; // set by receiver: The last timestamp of a received HeartBeatResponse. Used to detect timeout
} msg_relay_t;

// ************************************ PRIVATE FUNCTIONS ****************************************** //
// Polling Utility
uint8_t get_new_device();

// Thread-handling for communicating with devices
void communicate(uint8_t port_num);
void* relayer(void* relay_cast);
void relay_clean_up(msg_relay_t* relay);
void* sender(void* relay_cast);
void* receiver(void* relay_cast);

// Device communication
int send_message(msg_relay_t* relay, message_t* msg, int* transferred);
int receive_message(msg_relay_t* relay, message_t* msg);
int ping(msg_relay_t* relay);

// Utility
int64_t millis();

// ************************************ PUBLIC FUNCTIONS ****************************************** //

// Initialize data structures / connections
void init() {
    // Init logger
    logger_init(DEV_HANDLER);
	// Init shared memory
	// shm_init(DEV_HANDLER);
}

// Free memory and safely stop connections
void stop() {
    log_printf(INFO, "Ctrl+C pressed. Safely terminating program\n");
	// TODO: For each tracked lowcar device, disconnect from shared memory
    // uint32_t connected_devs = 0;
    // get_catalog(&connected_devs);
    // for (int i = 0; i < MAX_DEVICES; i++) {
    //     if ((1 << i) & connected_devs) {
    //         device_disconnect(i);
    //     }
    // }
	// TODO: Stop shared memory
	// shm_stop(DEV_HANDLER);
    // Stop logger
    logger_stop();
	exit(0);
}

/*
 * Detects when devices are connected and disconnected.
 * On lowcar device connect, connect to shared memory and spawn three threads to communicate with the device
 * On lowcar device disconnect, disconnects the device from shared memory.
 */
void poll_connected_devices() {
	// Poll for newly connected devices and open threads for them
    log_printf(INFO, "Polling now.\n");
	while (1) {
		uint8_t port_num = get_new_device();
        if (port_num == (uint8_t) -1) {
            // log_printf(DEBUG, "Couldn't find a new device\n");
            continue;
        }
        log_printf(INFO, "Starting communication with new device /dev/ttyACM%d\n", port_num);
        communicate(port_num);
    }
}

// ************************************ POLLING UTILITY ****************************************** //

// Bitmap indicating whether /dev/ttyACM* is used, where * is the *-th bit
uint32_t used_ports = 0;

/*
 * Returns the number of the port corresponding to the newly connected device
 */
uint8_t get_new_device() {
    // Check every port
    for (int i = 0; i < MAX_DEVICES; i++) {
        uint8_t port_unused = ((1 << i) & used_ports) == 0; // Checks if bit i is zero
        // If last time this function was called port i wasn't used
        if (port_unused) {
            // If that port is being used now, it's a new device
            char port_name[14];
            sprintf(port_name, "/dev/ttyACM%d", i);
            // log_printf(DEBUG, "Checking %s\n", port_name);
            if (access(port_name, F_OK) != -1) {
                // File exists
                log_printf(INFO, "Port /dev/ttyACM%d is new\n", i);
                // Set the bit to be on
                used_ports |= (1 << i);
                return i;
            }
        }
    }
    return -1;
}

// ************************************ THREAD-HANDLING ****************************************** //

/*
 * Opens threads for communication with a device
 * Three threads will be opened:
 *  relayer: Verifies device is lowcar and cancels threads when device disconnects/timesout
 *  sender: Sends changed data in shared memory and periodically sends HeartBeatRequests
 *  receiver: Receives data from the lowcar device and signals sender thread about HeartBeatRequests
 *
 * dev: The libusb_device to communicate with
 */
void communicate(uint8_t port_num) {
	msg_relay_t* relay = malloc(sizeof(msg_relay_t));
    relay->port_num = port_num;
    char port_name[15]; // Template size + 2 indices for port_number
    sprintf(port_name, "/dev/ttyACM%d", relay->port_num);
    log_printf(DEBUG, "Setting up threads for %s\n", port_name);
	//relay->file_descriptor = serialport_init(port_name, BAUD_RATE);
    relay->file_descriptor = serialport_init(port_name, BAUD_RATE);
    if (relay->file_descriptor == -1) {
        log_printf(ERROR, "Couldn't open port %s\n", port_name);
    } else {
        log_printf(DEBUG, "Opened port %s\n", port_name);
    }
	if (OUTPUT == FILE_DEV) {
        // Open the file to read from
        relay->read_file = fopen(TO_DEV_HANDLER, "w+");
        if (relay->read_file == NULL) {
            log_printf(ERROR, "Couldn't open TO_DEVICE file\n");
        }
        // Open the file to write to
        relay->write_file = fopen(TO_DEVICE, "w+");
        if (relay->write_file == NULL) {
            log_printf(ERROR, "Couldn't open TO_DEV_HANDLER file\n");
        }
    }
	// Initialize relay->start as 0, indicating sender and receiver to not start work yet
	relay->start = 0;
	// Initialize the other relay values
	relay->shm_dev_idx = -1;
	relay->dev_id.type = -1;
	relay->dev_id.year = -1;
	relay->dev_id.uid = -1;
    relay->got_hb_req = 0;
    relay->last_sent_hbreq_time = 0;
    relay->last_received_hbresp_time = 0;
	// Open threads for sender, receiver, and relayer
	pthread_create(&relay->sender, NULL, sender, relay);
	pthread_create(&relay->receiver, NULL, receiver, relay);
	pthread_create(&relay->relayer, NULL, relayer, relay);
    if (OUTPUT == FILE_DEV) {
        pthread_join(relay->relayer, NULL); // Let the threads do work. USB_DEV doesn't need this
    }
}

/*
 * Sends a Ping to the device and waits for a SubscriptionResponse
 * If the SubscriptionResponse takes too long, close the device and exit all threads
 * Connects the device to shared memory and signals the sender and receiver to start
 * Continuously checks if the device disconnected or HeartBeatRequest was sent without a response
 *      If so, it disconnects the device from shared memory, closes the device, and frees memory
 * RELAY_CAST is to be casted as msg_relay_t
 */
void* relayer(void* relay_cast) {
	msg_relay_t* relay = relay_cast;
	int ret;

	// Send a Ping and wait for a SubscriptionResponse
    log_printf(DEBUG, "Sending ping to /dev/ttyACM%d\n", relay->port_num);
	ret = ping(relay);
	if (ret != 0) {
		relay_clean_up(relay);
		return NULL;
	}

	/****** At this point, the Ping/SubscriptionResponse handshake was successful! ******/

	// TODO: Connect the lowcar device to shared memory
	log_printf(DEBUG, "Connecting /dev/ttyACM%d to shared memory\n", relay->port_num);
	// device_connect(relay->dev_id.type, relay->dev_id.year, relay->dev_id.uid, &relay->shm_dev_idx);

	// Signal the sender and receiver to start work
	relay->start = 1;

	// If the device disconnects or a HeartBeatRequest takes too long to be resolved, clean up
    log_printf(DEBUG, "Monitoring heartbeats with /dev/ttyACM%d\n", relay->port_num);
    char port_name[14];
    sprintf(port_name, "/dev/ttyACM%d", relay->port_num);
	while (1) {
		// If Arduino port file doesn't exist, it disconnected
        if (OUTPUT == USB_DEV && access(port_name, F_OK) == -1) {
            log_printf(INFO, "/dev/ttyACM%d disconnected!\n", relay->port_num);
            relay_clean_up(relay);
            return NULL;
        }
        /* If theres a pending HeartBeatRequest that wasn't resolved in time,
         * clean up.
         */
        uint8_t pending_request = relay->last_sent_hbreq_time > relay->last_received_hbresp_time;
		if (pending_request
                && ((millis() - relay->last_sent_hbreq_time) >= DEVICE_TIMEOUT)) {
			log_printf(INFO, "/dev/ttyACM%d timed out!\n", relay->port_num);
			relay_clean_up(relay);
			return NULL;
		}
	}
}

/* Called by relayer to clean up after the device.
 * Closes serialport, cancels threads,
 * disconnects from shared memory, and frees the RELAY struct
 */
void relay_clean_up(msg_relay_t* relay) {
    if (OUTPUT == USB_DEV) {
        serialport_close(relay->file_descriptor);
        used_ports &= ~(1 << relay->port_num); // Set bit to 0 to indicate unused
    }
	// Cancel the sender and receiver threads when ongoing transfers are completed
	pthread_cancel(relay->sender);
	pthread_cancel(relay->receiver);
	// Sleep to ensure that the sender and receiver threads are cancelled before disconnecting from shared memory / freeing relay
	sleep(1);
	// Disconnect the device from shared memory if it's connected
	if (relay->shm_dev_idx != -1) {
		// device_disconnect(relay->shm_dev_idx)
	}
    log_printf(DEBUG, "Cleaned up threads for /dev/ttyACM%d\n", relay->port_num);
	free(relay);
	pthread_cancel(pthread_self());
}

/*
 * Continuously reads from shared memory to serialize the necessary information
 * to send to the device.
 * Sets relay->sent_hb_req when a HeartBeatRequest is sent
 * Sends a HeartBeatResponse when relay->got_hb_req is set
 * RELAY_CAST is to be casted as msg_relay_t
 */
void* sender(void* relay_cast) {
	msg_relay_t* relay = relay_cast;
    log_printf(DEBUG, "Sender on standby\n");
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
	// pmap: Array of thirty-three uint32_t.
	//       Indices 1-32 are bitmaps indicating which parameters have new values to be written to the device at that index
	//       pmap[0] is a bitmap indicating which devices have new values to be written
	uint32_t* pmap;
	param_val_t* params = malloc(MAX_PARAMS * sizeof(param_val_t)); // Array of params to be filled on device_read
	message_t* msg; 		// Message to build
	int transferred = 0; 	// The number of bytes actually transferred
	int ret; 				// Hold the value from send_message()
    int8_t heartbeat_counter = -50;
    log_printf(DEBUG, "Sending starting work!\n");
	while (1) {
		// Get the current param bitmap
		// get_param_bitmap(pmap);
		/* If pmap[0] != 0, there are devices that need to be written to.
		 * This thread is interested in only whether or not THIS device requires being written to.
		 * If the device's shm_dev_idx bit is on in pmap[0], there are values to be written to the device. */
		if (relay->shm_dev_idx > 0 && (pmap[0] & (1 << relay->shm_dev_idx))) {
			// Read the new parameter values from shared memory as DEV_HANDLER from the COMMAND stream
			// device_read(relay->shm_dev_idx, DEV_HANDLER, COMMAND, pmap[1 + relay->shm_dev_idx], params);
			// Serialize and bulk transfer a DeviceWrite packet with PARAMS to the device
			msg = make_device_write(&relay->dev_id, pmap[1 + relay->shm_dev_idx], params);
			ret = send_message(relay, msg, &transferred);
			if (ret != 0) {
                log_printf(ERROR, "DeviceWrite transfer failed\n");
			}
			destroy_message(msg);
		}
		pthread_testcancel(); // Cancellation point
		// If it's been HB_REQ_FREQ milliseconds since last sending a HeartBeatRequest, send another one
		if ((millis() - relay->last_sent_hbreq_time) >= HB_REQ_FREQ) {
			msg = make_heartbeat_request(heartbeat_counter);
			ret = send_message(relay, msg, &transferred);
			if (ret != 0) {
                log_printf(ERROR, "HeartBeatRequest transfer failed\n");
			}
            log_printf(DEBUG, "Sent HeartBeatRequest: %d\n", heartbeat_counter);
            // Update the timestamp at which we sent a HeartBeatRequest
            relay->last_sent_hbreq_time = millis();
			destroy_message(msg);
            heartbeat_counter--;
		}
		pthread_testcancel(); // Cancellation point
		// If receiver got a HeartBeatRequest, send a HeartBeatResponse and mark the request as resolved
		if (relay->got_hb_req != 0) {
			msg = make_heartbeat_response(relay->got_hb_req);
			send_message(relay, msg, &transferred);
			if (ret != 0) {
                log_printf(ERROR, "HeartBeatResponse transfer failed\n");
			}
            log_printf(DEBUG, "Sent HeartBeatResponse: %d\n", relay->got_hb_req);
			destroy_message(msg);
            // Mark the HeartBeatRequest as resolved
			relay->got_hb_req = 0;
		}
		pthread_testcancel(); // Cancellation point
	}
	return NULL;
}

/*
 * Continuously attempts to parse incoming data over serial and send to shared memory
 * Sets relay->got_hb_req upon receiving a HeartBeatRequest
 * RELAY_CAST is to be casted as msg_relay_t
 */
void* receiver(void* relay_cast) {
	msg_relay_t* relay = relay_cast;
    log_printf(DEBUG, "Receiver on standby\n");
	// Sleep until relay->start == 1, which is true when relayer receives a SubscriptionResponse
	while (1) {
		if (relay->start == 1) {
			break;
		}
		pthread_testcancel(); // Cancellation point. Relayer may cancel this thread if device timesout
	}
    log_printf(DEBUG, "Receiver starting work!\n");
	/* Do work! Continuously read from the device until a complete message can be parsed.
	 * If it's a HeartBeatRequest, signal sender to reply with a HeartBeatResponse
	 * If it's a HeartBeatResponse, mark the current HeartBeatRequest as resolved
	 * If it's device data, write it to shared memory
	 */
	// An empty message to parse the received data into
	message_t* msg = make_empty(MAX_PAYLOAD_SIZE);
	// An array of empty parameter values to be populated from DeviceData message payloads and written to shared memory
	param_val_t* vals = malloc(MAX_PARAMS * sizeof(param_val_t));
	while (1) {
		pthread_testcancel(); // Cancellation point
		// Try to read a message
		if (receive_message(relay, msg) != 0) {
			continue;
		}
		if (msg->message_id == HeartBeatRequest) {
			// If it's a HeartBeatRequest, take note of it so sender can resolve it
            log_printf(DEBUG, "Got HeartBeatRequest: %d\n", (int8_t) msg->payload[0]);
			relay->got_hb_req = (int8_t) msg->payload[0];
		} else if (msg->message_id == HeartBeatResponse) {
			// If it's a HeartBeatResponse, mark the current HeartBeatRequest as resolved
            log_printf(DEBUG, "Got HeartBeatResponse: %d\n", (int8_t) msg->payload[0]);
			relay->last_received_hbresp_time = millis();
		} else if (msg->message_id == DeviceData) {
			// First, parse the payload into an array of parameter values
			parse_device_data(relay->dev_id.type, msg, vals); // This will overwrite the values in VALS that are indicated in the bitmap
			// TODO: Now write it to shared memory
			// device_write(relay->shm_dev_idx, DEV_HANDLER, DATA, bitmap, vals);
		} else if (msg->message_id == Log) {
            log_printf(INFO, "FROM /dev/ttyACM%d: %s\n", relay->port_num, msg->payload);
		} else if (msg->message_id == Error) {
            log_printf(ERROR, "FROM /dev/ttyACM%d: %s\n", relay->port_num, msg->payload);
		} else {
			// Received a message of unexpected type.
			log_printf(FATAL, "Received a message of unexpected type and dropping it.\n");
		}
		// Now that the message is taken care of, clear the message
		msg->message_id = 0x0;
		// TODO: Maybe empty the payload?
		msg->payload_length = 0;
	    msg->max_payload_length = MAX_PAYLOAD_SIZE;
	}
	return NULL;
}

// ************************************ DEVICE COMMUNICATION ****************************************** //

/*
 * Serializes, encodes, and sends a message
 * msg: The message to be sent, constructed from the functions below
 * transferred: Output location for number of bytes actually sent
 * return: 0 if successful, Otherwise,
 *         The return value of libusb_bulk_transfer if OUTPUT == USB_DEV
 *          http://libusb.sourceforge.net/api-1.0/group__libusb__syncio.html#gab8ae853ab492c22d707241dc26c8a805
 *          -1 if FILE_DEV and unsuccessful
 *
 */
int send_message(msg_relay_t* relay, message_t* msg, int* transferred) {
	int len = calc_max_cobs_msg_length(msg);
	uint8_t* data = malloc(len);
	len = message_to_bytes(msg, data, len);
    int ret;
    if (OUTPUT == USB_DEV) {
        for (int i = 0; i < len; i++) {
            ret = serialport_writebyte(relay->file_descriptor, data[i]); // From arduino-serial-lib
            if (ret != 0) {
                log_printf(DEBUG, "Couldn't write byte 0x%X\n", data[i]);
                break;
            }
        }
    } else if (OUTPUT == FILE_DEV) {
        // Write the data buffer to the file
        *transferred = fwrite(data, sizeof(uint8_t), len, relay->write_file);
        fflush(relay->write_file); // Force write the contents to file
        ret = (*transferred == len) ? 0 : -1;
    }
    if (ret == 0) {
        log_printf(DEBUG, "Sent %d bytes: ", len);
        print_bytes(data, len);
    }
    free(data);
	return ret;
}

/*
 * Continuously reads from stream until reads the next message, then attempts to parse
 * relay: The shred relay object
 * msg: The message_t* to be populated with the parsed data (if successful)
 * return: 0 on successful parse
 *         1 on broken message
 *         2 on incorrect checksum
 */
int receive_message(msg_relay_t* relay, message_t* msg) {
    int num_bytes_read = 0;
	// Variable to temporarily hold a read byte
	uint8_t last_byte_read;
    int ret;
	// Keep reading until we get the delimiter byte
    while (!(num_bytes_read == 1 && last_byte_read == 0)) {
        if (OUTPUT == USB_DEV) {
            num_bytes_read = read(relay->file_descriptor, &last_byte_read, 1); // Read a byte
        } else if (OUTPUT == FILE_DEV) {
            num_bytes_read = fread(&last_byte_read, sizeof(uint8_t), 1, relay->read_file);
        }
        // If we were able to read a byte but it wasn't the delimiter
        if (num_bytes_read == 1 && last_byte_read != 0) {
            log_printf(DEBUG, "Attempting to read delimiter but got 0x%X\n", last_byte_read);
        }
    }
    log_printf(DEBUG, "Got start of message!\n");

    // Try to read the length of the cobs encoded message (the next byte)
    uint8_t cobs_len;
    num_bytes_read = 0;
    while (num_bytes_read != 1) {
        if (OUTPUT == USB_DEV) {
            num_bytes_read = read(relay->file_descriptor, &cobs_len, 1);
        } else if (OUTPUT == FILE_DEV) {
            num_bytes_read = fread(&cobs_len, sizeof(uint8_t), 1, relay->read_file);
        }
    }
    log_printf(DEBUG, "Cobs len of message is %d\n", cobs_len);

    // Allocate buffer to read message into
    uint8_t* data = malloc(DELIMITER_SIZE + COBS_LENGTH_SIZE + cobs_len);
    data[0] = 0x00;
    data[1] = cobs_len;

    // Read the message
    if (OUTPUT == USB_DEV) {
        num_bytes_read = read(relay->file_descriptor, &data[2], cobs_len);
    } else if (OUTPUT == FILE_DEV) {
        num_bytes_read = fread(&data[2], sizeof(uint8_t), cobs_len, relay->read_file);
    }
    if (num_bytes_read != cobs_len) {
        log_printf(DEBUG, "Couldn't read the full message\n");
        free(data);
        return 1;
    }

    // Parse the message
    if (parse_message(data, msg) != 0) {
        log_printf(DEBUG, "Incorrect checksum\n");
        free(data);
        return 2;
    }
    log_printf(DEBUG, "Received %d bytes: ", 2 + cobs_len);
    print_bytes(data, 2 + cobs_len);
    free(data);
    return 0;
}

/*
 * Synchronously sends a PING message to a device to check if it's a lowcar device
 * relay: Struct holding device, handle, and endpoint fields
 * return: 0 upon successfully receiving a SubscriptionResponse and setting relay->dev_id
 *      1 if Ping message couldn't be sent
 *      2 SubscriptionResponse wasn't received
 */
int ping(msg_relay_t* relay) {
	// Send a Ping
    log_printf(DEBUG, "Sending a Ping...\n");
    message_t* ping = make_ping();
    int transferred;
    int ret = send_message(relay, ping, &transferred);
    destroy_message(ping);
    if (ret != 0) {
        return 1;
    }

	// Try to read a SubscriptionResponse, which we expect from a lowcar device that receives a Ping
    message_t* sub_response = make_empty(MAX_PAYLOAD_SIZE);
	log_printf(DEBUG, "Listening for SubscriptionResponse\n");
    while (receive_message(relay, sub_response) != 0) {
        sleep(1);
    }
    if (sub_response->message_id != SubscriptionResponse) {
		log_printf(DEBUG, "Message is not a SubscriptionResponse\n");
		destroy_message(sub_response);
		return 2;
	}

	// We have a SubscriptionResponse!
	relay->dev_id.type = ((uint16_t*)(&sub_response->payload[6]))[0];	// device type is the 16-bits starting at the 6th byte
	relay->dev_id.year = sub_response->payload[8];						// device year is the 8th byte
	relay->dev_id.uid  = ((uint64_t*)(&sub_response->payload[9]))[0];	// device uid is the 64-bits starting at the 9th byte
    log_printf(DEBUG, "SubscriptionResponse received! /dev/ttyACM%d is type %d, year %d, uid %d!\n", \
        relay->port_num, relay->dev_id.type, relay->dev_id.year, relay->dev_id.uid);
    destroy_message(sub_response);
	return 0;
}

// ************************************ UTILITY ****************************************** //

/* Returns the number of milliseconds since the Unix Epoch */
int64_t millis() {
	struct timeval time; // Holds the current time in seconds + microsecondsx
	gettimeofday(&time, NULL);
	int64_t s1 = (int64_t)(time.tv_sec) * 1000; // Convert seconds to milliseconds
	int64_t s2 = (time.tv_usec / 1000);			// Convert microseconds to milliseconds
	return s1 + s2;
}

// ************************************ MAIN ****************************************** //

int main() {
	// If SIGINT (Ctrl+C) is received, call sigintHandler to clean up
	signal(SIGINT, stop);
	init();
    log_printf(INFO, "DEV_HANDLER starting with OUTPUT==%s\n", (OUTPUT == USB_DEV) ? "USB_DEV" : "FILE_DEV");
    if (OUTPUT == USB_DEV) {
        // Start polling if OUTPUT == USB
        poll_connected_devices();
    } else if (OUTPUT == FILE_DEV) {
        // Skip polling
        communicate(-1);
    }
	return 0;
}

