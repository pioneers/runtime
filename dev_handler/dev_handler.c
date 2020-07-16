#include "dev_handler.h"

// ************************************ PRIVATE TYPEDEFS ****************************************** //
// The baud rate set on the Arduino
#define BAUD_RATE 115200
// The interval (ms) at which we want DEVICE_DATA messages for subscribed params
#define SUB_INTERVAL 500

/* Struct shared between the three threads so they can communicate with each other
 * relayer thread acts as the "control center" and connects/disconnects to shared memory
 *  relayer thread also frees memory when device is disconnected
 */
typedef struct msg_relay {
    pthread_t sender;
    pthread_t receiver;
    pthread_t relayer;
    uint8_t port_num;        // where port is "/dev/ttyACM<port_num>/"
    FILE* read_file;         // For use with OUTPUT == FILE_DEV
    FILE* write_file;        // For use with OUTPUT == FILE_DEV
    int file_descriptor;     // Obtained from opening port. Used to close port.
    int shm_dev_idx;         // The unique index assigned to the device by shm_wrapper for shared memory operations on device_connect()
    uint8_t start;		     // set by relayer: A flag to tell reader and receiver to start work when set to 1
    dev_id_t dev_id;         // set by relayer once ACKNOWLEDGEMENT is received
    uint64_t last_received_ping_time; // set by receiver: Timestamp of the most recent PING from the device
    uint64_t last_sent_ping_time;     // set by sender: Timestamp of the last sent PING to the device
} msg_relay_t;

// ************************************ PRIVATE FUNCTIONS ****************************************** //
// Polling Utility
int8_t get_new_device();

// Thread-handling for communicating with devices
void communicate(uint8_t port_num);
void* relayer(void* relay_cast);
void relay_clean_up(msg_relay_t* relay);
void* sender(void* relay_cast);
void* receiver(void* relay_cast);

// Device communication
int send_message(msg_relay_t* relay, message_t* msg);
int receive_message(msg_relay_t* relay, message_t* msg, uint16_t timeout);
int verify_lowcar(msg_relay_t* relay);

// Serial port opening and closing
int serialport_init(const char* serialport, int baud);
int serialport_close(int fd);

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
        sleep(1);   // Save CPU usage by sleeping for less than second
		int8_t port_num = get_new_device();
        if (port_num == -1) {
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
int8_t get_new_device() {
    // Check every port
    for (int i = 0; i < MAX_DEVICES; i++) {
        uint8_t port_unused = ((1 << i) & used_ports) == 0; // Checks if bit i is zero
        // If last time this function was called port i wasn't used
        if (port_unused) {
            char port_name[14];
            sprintf(port_name, "/dev/ttyACM%d", i);
            // If that port is being used now (file exists), it's a new device
            if (access(port_name, F_OK) != -1) {
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
 *  sender: Sends data to write to device and periodically sends PING
 *  receiver: Receives parameter data from the lowcar device and processes logs
 *
 * port_num: The port number of the new device to connect to
 */
void communicate(uint8_t port_num) {
	msg_relay_t* relay = malloc(sizeof(msg_relay_t));
    relay->port_num = port_num;
    if (OUTPUT == USB_DEV) {
        char port_name[15]; // Template size + 2 indices for port_number
        sprintf(port_name, "/dev/ttyACM%d", relay->port_num);
        log_printf(DEBUG, "Setting up threads for %s\n", port_name);
        relay->file_descriptor = serialport_init(port_name, BAUD_RATE);
        if (relay->file_descriptor == -1) {
            log_printf(ERROR, "Couldn't open port %s\n", port_name);
            free(relay);
            return;
        } else {
            log_printf(DEBUG, "Opened port %s\n", port_name);
        }
    } else if (OUTPUT == FILE_DEV) {
        // Open the file to read from
        relay->read_file = fopen(TO_DEV_HANDLER, "w+");
        if (relay->read_file == NULL) {
            log_printf(ERROR, "Couldn't open file to read from\n");
        }
        // Open the file to write to
        relay->write_file = fopen(TO_DEVICE, "w+");
        if (relay->write_file == NULL) {
            log_printf(ERROR, "Couldn't open file to write to\n");
        }
    }
	// Initialize relay->start as 0, indicating sender and receiver to not start work yet
	relay->start = 0;
	// Initialize the other relay values
	relay->shm_dev_idx = -1;
	relay->dev_id.type = -1;
	relay->dev_id.year = -1;
	relay->dev_id.uid = -1;
    relay->last_sent_ping_time = 0;
    relay->last_received_ping_time = 0;
	// Open threads for sender, receiver, and relayer
	pthread_create(&relay->sender, NULL, sender, relay);
	pthread_create(&relay->receiver, NULL, receiver, relay);
	pthread_create(&relay->relayer, NULL, relayer, relay);
    if (OUTPUT == FILE_DEV) {
        pthread_join(relay->relayer, NULL); // Let the threads do work. USB_DEV doesn't need this
    }
}

/*
 * Sends a PING to the device and waits for an ACKNOWLEDGEMENT
 * If the ACKNOWLEDGEMENT takes too long, close the device and exit all threads
 * Connects the device to shared memory and signals the sender and receiver to start
 * Continuously checks if the device disconnected or timed out
 *      If so, it disconnects the device from shared memory, closes the device, and frees memory
 * RELAY_CAST is to be casted as msg_relay_t
 */
void* relayer(void* relay_cast) {
	msg_relay_t* relay = relay_cast;
	int ret;

	// Verify that the device is a lowcar device
    log_printf(DEBUG, "Verifying that /dev/ttyACM%d is lowcar\n", relay->port_num);
	ret = verify_lowcar(relay);
	if (ret != 0) {
        log_printf(INFO, "/dev/ttyACM%d couldn't be verified to be a lowcar device", relay->port_num);
		relay_clean_up(relay);
		return NULL;
	}

	/****** At this point, the device is confirmed to be lowcar! ******/

	// TODO: Connect the lowcar device to shared memory
	log_printf(DEBUG, "Connecting %s to shared memory\n", get_device_name(relay->dev_id.type));
	// device_connect(relay->dev_id.type, relay->dev_id.year, relay->dev_id.uid, &relay->shm_dev_idx);

	// Signal the sender and receiver to start work
	relay->start = 1;

    // TODO: Subscribe to only params requested in shared memory
    // uint32_t sub_map[MAX_DEVICES + 1];
    // get_sub_requests(sub_map);
    // if (sub_map[0] & (1 << relay->shm_dev_idx)) {
    //     message_t* sub_request = make_subscription_request(relay->dev_id.type, sub_map[1 + relay->shm_dev_idx], SUB_INTERVAL);
    //     ret = send_message(relay, sub_request);
    //     if (ret != 0) {
    //         log_printf(FATAL, "Couldn't send initial SUBSCRIPTION_REQUEST to %s", get_device_name(relay->dev_id.type);
    //     }
    //     destroy_message(sub_request);
    // }

    // Subscribe to all params
    message_t* sub_request = make_subscription_request(relay->dev_id.type, (uint32_t) -1, SUB_INTERVAL);
    ret = send_message(relay, sub_request);
    if (ret != 0) {
        log_printf(FATAL, "Couldn't send initial SUBSCRIPTION_REQUEST to %s", get_device_name(relay->dev_id.type));
    }
    destroy_message(sub_request);

	// If the device disconnects or times out, clean up
    log_printf(DEBUG, "Relayer monitoring %s", get_device_name(relay->dev_id.type));
    char port_name[14];
    sprintf(port_name, "/dev/ttyACM%d", relay->port_num);
	while (1) {
		// If Arduino port file doesn't exist, it disconnected
        if (OUTPUT == USB_DEV && access(port_name, F_OK) == -1) {
            log_printf(INFO, "%s disconnected!", get_device_name(relay->dev_id.type));
            relay_clean_up(relay);
            return NULL;
        }
        /* If it took too long to receive a Ping, the device timed out */
		if ((millis() - relay->last_received_ping_time) >= TIMEOUT) {
			log_printf(INFO, "%s timed out!", get_device_name(relay->dev_id.type));
			relay_clean_up(relay);
			return NULL;
		}
		sleep(1);
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
    log_printf(DEBUG, "Cleaned up threads for %s", get_device_name(relay->dev_id.type));
	free(relay);
	pthread_cancel(pthread_self());
}

/*
 * Continuously reads from shared memory to serialize the necessary information
 * to send to the device.
 * Sets relay->last_sent_hb_time when a HeartBeatRequest is sent
 * RELAY_CAST is to be casted as msg_relay_t
 */
void* sender(void* relay_cast) {
	msg_relay_t* relay = relay_cast;
    log_printf(DEBUG, "Sender on standby\n");
	// Sleep until relay->start == 1, (when relayer gets ACKNOWLEDGEMENT)
	while (1) {
		if (relay->start == 1) {
			break;
		}
		pthread_testcancel(); // Cancellation point. Relayer may cancel this thread if device timesout
	}

    // Cancel this thread only where pthread_testcancel()
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

	/* Do work!
     * Continuously call get_cmd_map(). If bit i in pmap[0] != 0, there are values to write to device i
     * The bitmap of params that need to be written to are at pmap[1 + i]
     * Use shared memory device_read() with pmap[1 + i] to get the values to write
     *
	 * Send PING at a regular interval, PING_FREQ.
     *
     * Continuously call get_sub_requests().
     * If bit i in sub_map[0] != 0, there is a new SUBSCRIPTION_REQUEST to be sent to device i
     * The bitmap of params to be subscribed is sub_map[1 + i]
     */
	uint32_t pmap[MAX_DEVICES + 1];
	param_val_t* params = malloc(MAX_PARAMS * sizeof(param_val_t)); // Array of params to be filled on device_read()
    uint32_t sub_map[MAX_DEVICES + 1];
	message_t* msg; 		// Message to build
	int ret; 				// Hold the value from send_message()
    log_printf(DEBUG, "Sender for %s starting work!", get_device_name(relay->dev_id.type));
	while (1) {
		// TODO: Write to device if needed
		/* get_cmd_map(pmap);
		if (pmap[0] & (1 << relay->shm_dev_idx)) {
			// Read the new parameter values from shared memory as DEV_HANDLER from the COMMAND stream
			device_read(relay->shm_dev_idx, DEV_HANDLER, COMMAND, pmap[1 + relay->shm_dev_idx], params);
			// Serialize and bulk transfer a DeviceWrite packet with PARAMS to the device
			msg = make_device_write(relay->dev_id.type, pmap[1 + relay->shm_dev_idx], params);
			ret = send_message(relay, msg);
			if (ret != 0) {
				log_printf(FATAL, "Couldn't send DEVICE_WRITE to %s", get_device_name(relay->dev_id.type));
			}
			destroy_message(msg);
		} */

		// Send another PING if it's time again
		if ((millis() - relay->last_sent_ping_time) >= PING_FREQ) {
			msg = make_ping();
			ret = send_message(relay, msg);
			if (ret != 0) {
				log_printf(FATAL, "Couldn't send PING to %s", get_device_name(relay->dev_id.type));
			}
			// Update the timestamp at which we sent a PING
			relay->last_sent_ping_time = millis();
			destroy_message(msg);
		}

        // TOOD: Send another SUBSCRIPTION_REQUEST if requested
        /*
        get_sub_requests(sub_map);
        if (sub_map[0] & (1 << relay->shm_dev_idx)) {
            msg = make_subscription_request(relay->dev_id.type, sub_map[1 + relay->shm_dev_idx], SUB_INTERVAL);
            ret = send_message(relay, msg);
            if (ret != 0) {
                log_printf(FATAL, "Couldn't send SUBSCRIPTION_REQUEST to %s", get_device_name(relay->dev_id.type));
            }
            destroy_message(msg);
        }
        */

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_testcancel(); // Cancellation point
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		usleep(1000);
	}
	return NULL;
}

/*
 * Continuously attempts to parse incoming data over serial and send to shared memory
 * Sets relay->last_received_ping_time upon receiving a PING
 * RELAY_CAST is to be casted as msg_relay_t
 */
void* receiver(void* relay_cast) {
	msg_relay_t* relay = relay_cast;
    log_printf(DEBUG, "Receiver on standby\n");
	// Sleep until relay->start == 1, (when relayer gets an ACKNOWLEDGEMENT)
	while (1) {
		if (relay->start == 1) {
			break;
		}
		pthread_testcancel(); // Cancellation point. Relayer may cancel this thread if device timesout
	}
    log_printf(DEBUG, "Receiver for %s starting work!", get_device_name(relay->dev_id.type));

    // Cancel this thread only where pthread_testcancel()
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

	/* Do work! Continuously read from the device until a complete message can be parsed.
	 * If it's a PING, update relay->last_received_ping_time
	 * If it's DEVICE_DATA, write it to shared memory
	 */
	// An empty message to parse the received data into
	message_t* msg = make_empty(MAX_PAYLOAD_SIZE);
	// An array of empty parameter values to be populated from DeviceData message payloads and written to shared memory
	param_val_t* vals = malloc(MAX_PARAMS * sizeof(param_val_t));
	while (1) {
		// Try to read a message
		if (receive_message(relay, msg, 0) != 0) {
			continue;
		}
		if (msg->message_id == PING) {
			// If it's a Ping, take note of it so sender can resolve it
            log_printf(DEBUG, "Got PING from %s", get_device_name(relay->dev_id.type));
			relay->last_received_ping_time = millis();
		} else if (msg->message_id == DEVICE_DATA) {
            //printf("Got DEVICE DATA from %s\n", get_device_name(relay->dev_id.type));
		    log_printf(DEBUG, "Got DEVICE_DATA from %s", get_device_name(relay->dev_id.type));
			// First, parse the payload into an array of parameter values
			parse_device_data(relay->dev_id.type, msg, vals); // This will overwrite the values in VALS that are indicated in the bitmap
			// TODO: Now write it to shared memory
			// device_write(relay->shm_dev_idx, DEV_HANDLER, DATA, bitmap, vals);
		} else if (msg->message_id == LOG) {
            log_printf(INFO, "[%s]: %s\n", get_device_name(relay->dev_id.type), msg->payload);
		} else {
			log_printf(FATAL, "Dropping received message of unexpected type from %s", get_device_name(relay->dev_id.type));
		}
		// Now that the message is taken care of, clear the message
		msg->message_id = 0x0;
		// TODO: Maybe empty the payload?
		msg->payload_length = 0;
	    msg->max_payload_length = MAX_PAYLOAD_SIZE;

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_testcancel(); // Cancellation point
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	}
	return NULL;
}

// ************************************ DEVICE COMMUNICATION ****************************************** //

/*
 * Serializes, encodes, and sends a message
 * msg: The message to be sent, constructed from the functions below
 * transferred: Output location for number of bytes actually sent
 * return: 0 if successful
 *         -1 if couldn't write all the bytes
 */
int send_message(msg_relay_t* relay, message_t* msg) {
	int len = calc_max_cobs_msg_length(msg);
	uint8_t* data = malloc(len);
	len = message_to_bytes(msg, data, len);
    int transferred = 0;
    if (OUTPUT == USB_DEV) {
        transferred = write(relay->file_descriptor, data, len);
    } else if (OUTPUT == FILE_DEV) {
        // Write the data buffer to the file
        transferred = fwrite(data, sizeof(uint8_t), len, relay->write_file);
        fflush(relay->write_file); // Force write the contents to file
    }
    if (transferred != len) {
        log_printf(FATAL, "Sent only %d out of %d bytes\n", transferred, len);
    }
    free(data);
    //log_printf(DEBUG, "Sent %d bytes: ", len);
    //print_bytes(data, len);
	return (transferred == len) ? 0 : -1;
}

/*
 * Continuously reads from stream until reads the next message, then attempts to parse
 * This function blocks until it reads a (possibly broken) message
 * relay: The shred relay object
 * msg: The message_t* to be populated with the parsed data (if successful)
 * timeout: The maximum seconds to wait until there's something to read
 *      Set TIMEOUT to 0 if we wish to keep waiting forever
 * return: 0 on successful parse
 *         1 on broken message
 *         2 on incorrect checksum
 *         3 on timeout
 */
int receive_message(msg_relay_t* relay, message_t* msg, uint16_t timeout) {
	int num_bytes_read = 0;
	// Variable to temporarily hold a read byte
	uint8_t last_byte_read;
	int ret;
	// Keep reading a byte until we get the delimiter byte
	while (!(num_bytes_read == 1 && last_byte_read == 0x00)) {
		if (OUTPUT == USB_DEV) {
			// Wait until there's something to read for up to TIMEOUT microseconds \\ https://stackoverflow.com/a/2918709
			if (timeout > 0) {
				struct timeval timeout_struct;
				timeout_struct.tv_sec = timeout;
				timeout_struct.tv_usec = 0;
    			fd_set read_set;
    			FD_ZERO(&read_set);
    			FD_SET(relay->file_descriptor, &read_set);
    			if (select(relay->file_descriptor + 1, &read_set, NULL, NULL, &timeout_struct) == 0) {
    				if (!FD_ISSET(relay->file_descriptor, &read_set)) {
    					return 3;
    				}
    			}
            }
			num_bytes_read = read(relay->file_descriptor, &last_byte_read, 1);
		} else if (OUTPUT == FILE_DEV) {
			num_bytes_read = fread(&last_byte_read, sizeof(uint8_t), 1, relay->read_file);
		}
		// If we were able to read a byte but it wasn't the delimiter
		if (num_bytes_read == 1 && last_byte_read != 0) {
			log_printf(DEBUG, "Attempting to read delimiter but got 0x%X\n", last_byte_read);
			num_bytes_read = 0;
		}
		usleep(1000);
	}
	//log_printf(DEBUG, "Got start of message!\n");

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
	//log_printf(DEBUG, "Cobs len of message is %d\n", cobs_len);

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

	//log_printf(DEBUG, "Received %d bytes: ", 2 + cobs_len);
	//print_bytes(data, 2 + cobs_len);

	// Parse the message
	ret = parse_message(data, msg);
	free(data);
	if (ret != 0) {
		log_printf(DEBUG, "Incorrect checksum\n");
		return 2;
	}
	return 0;
}

/*
 * Synchronously sends a PING message to a device to check if it's a lowcar device
 * relay: Struct holding device, handle, and endpoint fields
 * return:  0 upon receiving an ACKNOWLEDGEMENT. Sets relay->dev_id
 *          1 if Ping message couldn't be sent
 *          2 ACKNOWLEDGEMENT wasn't received
 */
int verify_lowcar(msg_relay_t* relay) {
	// Send a Ping
    log_printf(DEBUG, "Sending a Ping...\n");
    message_t* ping = make_ping();
    int ret = send_message(relay, ping);
    destroy_message(ping);
    if (ret != 0) {
        return 1;
    }

	// Try to read an ACKNOWLEDGEMENT, which we expect from a lowcar device that receives a PING
    message_t* ack = make_empty(MAX_PAYLOAD_SIZE);
	log_printf(DEBUG, "Listening for ACKNOWLEDGEMENT from /dev/ttyACM%d\n", relay->port_num);
    while (1) {
        ret = receive_message(relay, ack, (uint16_t) (TIMEOUT / 1000));
        if (ret == 0) { // Got message
            break;
        } else if (ret == 3) { // Timeout
            return 2;
        }
        sleep(1);
    }
    if (ack->message_id != ACKNOWLEDGEMENT) {
		log_printf(DEBUG, "Message is not an ACKNOWLEDGEMENT, but of type %d", ack->message_id);
		destroy_message(ack);
		return 2;
	}

	// We have a lowcar device!
	relay->dev_id.type = *((uint16_t*)(&ack->payload[0]));      // device type is the first 16 bits
	relay->dev_id.year = ack->payload[2];                    // device year is next 8 bits
	relay->dev_id.uid  = *((uint64_t*)(&ack->payload[3]));   // device uid is the last 64-bits
    log_printf(DEBUG, "ACKNOWLEDGEMENT received! /dev/ttyACM%d is type 0x%04X (%s), year 0x%02X, uid 0x%llX!\n", \
        relay->port_num, relay->dev_id.type, get_device_name(relay->dev_id.type), relay->dev_id.year, relay->dev_id.uid);
	relay->last_received_ping_time = millis();
    destroy_message(ack);
	return 0;
}

// ************************************ SERIAL PORTS ****************************************** //

/**
 * Opens a serial port for reading and writing binary data
 * serialport: The name of the port (ex: "/dev/ttyACM0", "/dev/tty.usbserial", "COM1")
 * baud: The baud rate of the connection
 * returns a file_descriptor or -1 on error
 *
 * source: https://github.com/todbot/arduino-serial
 */
int serialport_init(const char* serialport, int baud) {
    struct termios toptions;
    int fd = open(serialport, O_RDWR);

    if (fd == -1)  {
        log_printf(ERROR, "serialport_init: Unable to open port");
        return -1;
    }

    if (tcgetattr(fd, &toptions) < 0) {
        log_printf(ERROR, "serialport_init: Couldn't get term attributes");
        return -1;
    }
    speed_t brate = B115200;
    cfsetispeed(&toptions, brate);
    cfsetospeed(&toptions, brate);

    // set toptions: https://linux.die.net/man/3/cfsetspeed
    toptions.c_cflag &= ~PARENB;
    toptions.c_cflag &= ~CSTOPB;
    toptions.c_cflag &= ~CSIZE;
    toptions.c_cflag |= CS8;

    toptions.c_cflag &= ~CRTSCTS;
    toptions.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
    toptions.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL); // turn off s/w flow ctrl

    toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    toptions.c_oflag &= ~OPOST; // make raw

    // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
    toptions.c_cc[VMIN]  = 0;
    toptions.c_cc[VTIME] = 0;

    tcsetattr(fd, TCSANOW, &toptions);
    if( tcsetattr(fd, TCSAFLUSH, &toptions) < 0) {
        log_printf(ERROR, "init_serialport: Couldn't set term attributes");
        return -1;
    }

    return fd;
}

/**
 * Closes the serial port using the file descriptor obtained from serialport_init()
 */
int serialport_close(int fd) {
    return close(fd);
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
