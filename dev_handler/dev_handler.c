#include "dev_handler.h"

// ************************************ PRIVATE TYPEDEFS ****************************************** //
// The baud rate set on the Arduino
#define BAUD_RATE 115200
// The interval (ms) at which we want DEVICE_DATA messages for subscribed params
#define SUB_INTERVAL 1

/* Struct shared between the three threads so they can communicate with each other
 * relayer thread acts as the "control center" and connects/disconnects to shared memory
 * relayer thread also frees memory when device is disconnected
 */
typedef struct msg_relay {
    pthread_t sender;
    pthread_t receiver;
    pthread_t relayer;
    uint8_t port_num;        // where port is "/dev/ttyACM<port_num>/"
    int file_descriptor;     // Obtained from opening port. Used to close port.
    int shm_dev_idx;         // The unique index assigned to the device by shm_wrapper for shared memory operations on device_connect()
    dev_id_t dev_id;         // set by relayer once ACKNOWLEDGEMENT is received
    uint64_t last_received_ping_time; // set by receiver: Timestamp of the most recent PING from the device
    pthread_mutex_t relay_lock;
    pthread_cond_t start_cond;  // Conditional variable to broadcast to sender and receiver to start work
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
int receive_message(msg_relay_t* relay, message_t* msg);
int verify_lowcar(msg_relay_t* relay);

// Serial port opening and closing
int serialport_init(const char* serialport, int baud);
int serialport_close(int fd);

// Utility
uint64_t millis();
int readn (int fd, void *buf, uint16_t n);
int writen (int fd, void *buf, uint16_t n);

// ************************************ GLOBAL VARIABLES ****************************************** //
// Bitmap indicating whether /dev/ttyACM* is used, where * is the *-th bit
uint32_t used_ports = 0;
pthread_mutex_t used_ports_lock;

// ************************************ PUBLIC FUNCTIONS ****************************************** //

// Initialize data structures / connections
void init() {
    // Init logger
    logger_init(DEV_HANDLER);
	// Init shared memory
	shm_init();
    // Initialize lock on global variable USED_PORTS
    if (pthread_mutex_init(&used_ports_lock, NULL) != 0) {
        log_printf(ERROR, "Couldn't init USED_PORTS_LOCK");
	exit(1);
    }
}

// Free memory and safely stop connections
void stop() {
    log_printf(INFO, "Ctrl+C pressed. Safely terminating program\n");
	// For each tracked lowcar device, disconnect from shared memory
     uint32_t connected_devs = 0;
     get_catalog(&connected_devs);
     for (int i = 0; i < MAX_DEVICES; i++) {
         if ((1 << i) & connected_devs) {
             device_disconnect(i);
         }
     }
         //Stop shared memory
	 shm_stop();
    // Stop logger
    logger_stop();
    // Destroy locks
    pthread_mutex_destroy(&used_ports_lock);
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

/*
 * Returns the number of the port corresponding to the newly connected device
 */
int8_t get_new_device() {
    char port_name[14];
    // Check every port
    for (int i = 0; i < MAX_DEVICES; i++) {
        // If last time this function was called port i wasn't used
        pthread_mutex_lock(&used_ports_lock);
        if (!(used_ports & (1 << i))) { // Check if i-th bit of USED_PORTS is zero
            sprintf(port_name, "/dev/ttyACM%d", i);
            // If that port is being used now (file exists), it's a new device
            if (access(port_name, F_OK) != -1) {
                log_printf(INFO, "Port /dev/ttyACM%d is new\n", i);
                // Set the bit to be on
                used_ports |= (1 << i);
		pthread_mutex_unlock(&used_ports_lock);
                return i;
            }
        }
        pthread_mutex_unlock(&used_ports_lock);
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
    }

	// Initialize the other relay values
	relay->shm_dev_idx = -1;
	relay->dev_id.type = -1;
	relay->dev_id.year = -1;
	relay->dev_id.uid = -1;
    relay->last_received_ping_time = 0;
    pthread_mutex_init(&relay->relay_lock, NULL);
    pthread_cond_init(&relay->start_cond, NULL);

	// Open threads for sender, receiver, and relayer
    if (pthread_create(&relay->sender, NULL, sender, relay) != 0) {
        log_printf(ERROR, "Couldn't spawn thread for SENDER");
    }
    if (pthread_create(&relay->receiver, NULL, receiver, relay) != 0) {
        log_printf(ERROR, "Couldn't spawn thread for RECEIVER");
    }
    if (pthread_create(&relay->relayer, NULL, relayer, relay) != 0) {
        log_printf(ERROR, "Couldn't spawn thread for RELAYER");
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

	// Connect the lowcar device to shared memory
	log_printf(DEBUG, "Connecting %s to shared memory\n", get_device_name(relay->dev_id.type));
	device_connect(relay->dev_id, &relay->shm_dev_idx);

	// Broadcast to the sender and receiver to start work
    pthread_cond_broadcast(&relay->start_cond);

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
        pthread_mutex_lock(&relay->relay_lock);
		if ((millis() - relay->last_received_ping_time) >= TIMEOUT) {
			pthread_mutex_unlock(&relay->relay_lock);
			log_printf(INFO, "%s timed out!", get_device_name(relay->dev_id.type));
			relay_clean_up(relay);
			return NULL;
		}
        pthread_mutex_unlock(&relay->relay_lock);
		sleep(1);
	}
}

/* Called by relayer to clean up after the device.
 * Closes serialport, cancels threads,
 * disconnects from shared memory, and frees the RELAY struct
 */
void relay_clean_up(msg_relay_t* relay) {
	int ret;

	// Cancel the sender and receiver threads when ongoing transfers are completed
	pthread_cancel(relay->sender);
	pthread_cancel(relay->receiver);

    if ((ret = pthread_join(relay->sender, NULL)) != 0) {
        log_printf(ERROR, "pthread_join on relay->sender failed with error code %d", ret);
    }
    if ((ret = pthread_join(relay->receiver, NULL)) != 0) {
        log_printf(ERROR, "pthread_join on relay->receiver failed with error code %d", ret);
    }

	// Disconnect the device from shared memory if it's connected
	if (relay->shm_dev_idx != -1) {
		device_disconnect(relay->shm_dev_idx);
	}

	serialport_close(relay->file_descriptor);
	if (ret = pthread_mutex_lock(&used_ports_lock)) {
		log_printf(ERROR, "mutex lock failed with code %d", ret);
	}
    used_ports &= ~(1 << relay->port_num); // Set bit to 0 to indicate unused
    pthread_mutex_unlock(&used_ports_lock);

    pthread_mutex_destroy(&relay->relay_lock);
    pthread_cond_destroy(&relay->start_cond);

    log_printf(DEBUG, "Cleaned up threads for %s", get_device_name(relay->dev_id.type));
	free(relay);
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

    // Wait until relayer gets an ACKNOWLEDGEMENT
    pthread_mutex_lock(&relay->relay_lock);
    pthread_cond_wait(&relay->start_cond, &relay->relay_lock);
    pthread_mutex_unlock(&relay->relay_lock);

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
    uint64_t last_sent_ping_time = millis();
	while (1) {
		// Write to device if needed
		 get_cmd_map(pmap);
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
		}

		// Send another PING if it's time again
		if ((millis() - last_sent_ping_time) >= PING_FREQ) {
			msg = make_ping();
			ret = send_message(relay, msg);
			if (ret != 0) {
				log_printf(FATAL, "Couldn't send PING to %s", get_device_name(relay->dev_id.type));
			}
			// Update the timestamp at which we sent a PING
			last_sent_ping_time = millis();
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

    // Wait until relayer gets an ACKNOWLEDGEMENT
    pthread_mutex_lock(&relay->relay_lock);
    pthread_cond_wait(&relay->start_cond, &relay->relay_lock);
    pthread_mutex_unlock(&relay->relay_lock);

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
		if (receive_message(relay, msg) != 0) {
			continue;
		}
		if (msg->message_id == PING) {
			// If it's a Ping, take note of it so sender can resolve it
            //log_printf(DEBUG, "Got PING from %s", get_device_name(relay->dev_id.type));
            pthread_mutex_lock(&relay->relay_lock);
			relay->last_received_ping_time = millis();
            pthread_mutex_unlock(&relay->relay_lock);
		} else if (msg->message_id == DEVICE_DATA) {
            	//printf("Got DEVICE DATA from %s\n", get_device_name(relay->dev_id.type));
		    //log_printf(DEBUG, "Got DEVICE_DATA from %s", get_device_name(relay->dev_id.type));
			// First, parse the payload into an array of parameter values
			parse_device_data(relay->dev_id.type, msg, vals); // This will overwrite the values in VALS that are indicated in the bitmap
			// Now write it to shared memory
			device_write(relay->shm_dev_idx, DEV_HANDLER, DATA, *((uint32_t *)msg->payload), vals);
		} else if (msg->message_id == LOG) {
            log_printf(INFO, "[%s]: %s", get_device_name(relay->dev_id.type), msg->payload);
		} else {
			log_printf(FATAL, "Dropping received message of unexpected type from %s", get_device_name(relay->dev_id.type));
		}
		// Now that the message is taken care of, clear the message
		msg->message_id = 0x0;
		msg->payload_length = 0;
	    msg->max_payload_length = MAX_PAYLOAD_SIZE;
		memset(msg->payload, 0, MAX_PAYLOAD_SIZE);
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
        transferred = writen(relay->file_descriptor, data, len);
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
int receive_message(msg_relay_t* relay, message_t* msg) {
	// Variable to temporarily hold a read byte
	uint8_t last_byte_read = 0;;
	int ret;
	// Keep reading a byte until we get the delimiter byte
	while (1) {
		readn(relay->file_descriptor, &last_byte_read, 1);
		if (last_byte_read == 0x00) {
			break;
		}
		// If we were able to read a byte but it wasn't the delimiter
		log_printf(DEBUG, "Attempting to read delimiter but got 0x%X\n", last_byte_read);
	}
	//log_printf(DEBUG, "Got start of message!\n");

	// Try to read the length of the cobs encoded message (the next byte)
	uint8_t cobs_len;
	int num_bytes_read = 0;
	while (num_bytes_read != 1) {
		num_bytes_read = readn(relay->file_descriptor, &cobs_len, 1);
	}
	//log_printf(DEBUG, "Cobs len of message is %d\n", cobs_len);

	// Allocate buffer to read message into
	uint8_t* data = malloc(DELIMITER_SIZE + COBS_LENGTH_SIZE + cobs_len);
	data[0] = 0x00;
	data[1] = cobs_len;

	// Read the message
	if (OUTPUT == USB_DEV) {
		num_bytes_read = readn(relay->file_descriptor, &data[2], cobs_len);
	}
	if (num_bytes_read != cobs_len) {
		log_printf(DEBUG, "Couldn't read the full message. Read %d out of %d bytes\n", num_bytes_read, cobs_len);
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
        ret = receive_message(relay, ack);
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

    // Set serial port options to allow read() to block indefinitely
    struct termios toptions;
    if (tcgetattr(relay->file_descriptor, &toptions) < 0) { // Get current options
        log_printf(ERROR, "serialport_init: Couldn't get term attributes");
        return -1;
    }
    toptions.c_cc[VMIN]  = 1;               // read() must read at least a byte before returning
    // Save changes to TOPTIONS immediately using flag TCSANOW
    tcsetattr(relay->file_descriptor, TCSANOW, &toptions);
    if(tcsetattr(relay->file_descriptor, TCSAFLUSH, &toptions) < 0) {
        log_printf(ERROR, "init_serialport: Couldn't set term attributes");
        return -1;
    }

	// We have a lowcar device!
	relay->dev_id.type = *((uint16_t*)(&ack->payload[0]));      // device type is the first 16 bits
	relay->dev_id.year = ack->payload[2];                    // device year is next 8 bits
	relay->dev_id.uid  = *((uint64_t*)(&ack->payload[3]));   // device uid is the last 64-bits
    log_printf(DEBUG, "ACKNOWLEDGEMENT received! /dev/ttyACM%d is type 0x%04X (%s), year 0x%02X, uid 0x%llX!\n", \
        relay->port_num, relay->dev_id.type, get_device_name(relay->dev_id.type), relay->dev_id.year, relay->dev_id.uid);
	relay->last_received_ping_time = millis(); // Treat the ACK as a ping to prevent timeout
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
    // Open the serialport for reading and writing
    int fd = open(serialport, O_RDWR);
    if (fd == -1)  {
        log_printf(ERROR, "serialport_init: Unable to open port");
        return -1;
    }

    // Fill TOPTIONS with the current params for the serialport
    if (tcgetattr(fd, &toptions) < 0) {
        log_printf(ERROR, "serialport_init: Couldn't get term attributes");
        return -1;
    }

    // Set the baudrate of communication to 115200 (same as on Arduino)
    speed_t brate = B115200;
    cfsetispeed(&toptions, brate);
    cfsetospeed(&toptions, brate);

    // set toptions: https://linux.die.net/man/3/cfsetspeed
    // Set serial port config to 8-N-1, which is default for Arduino Serial.begin()
    toptions.c_cflag &= ~CSIZE;     // Reset character size
    toptions.c_cflag |= CS8;        // Set character size to 8
    toptions.c_cflag &= ~PARENB;    // Disable parity generation on output and parity checking for input (N)
    toptions.c_cflag &= ~CSTOPB;    // Set only one stop bit (1)

    // Turn off translating carriage return to newline on input
    // Without disabling this, all incoming 0x0D (\cr) would be read as 0x0A (\n) instead
    //toptions.c_iflag &= ~ICRNL;

    // TODO: Review these. Consider cfsetraw()
    //toptions.c_cflag &= ~CRTSCTS;                   // Disable RTS/CTS flow control TODO
    //toptions.c_cflag |= CREAD | CLOCAL;             // Enable receiver and ignore modem control lines
    //toptions.c_iflag &= ~(IXON | IXOFF | IXANY);    // Disable XON/XOFF flow control on output and input; TODO

    //toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    //toptions.c_oflag &= ~OPOST; // make raw

    cfmakeraw(&toptions);

    // Set options for read(fd, buffer, num_bytes_to_read)
    // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
    toptions.c_cc[VMIN]  = 0;               // Until receiving ACK, do not block indefinitely (use timeout)
    toptions.c_cc[VTIME] = TIMEOUT / 100;   // Number of deciseconds to timeout

    // Save changes to TOPTIONS immediately using flag TCSANOW
    tcsetattr(fd, TCSANOW, &toptions);
    if(tcsetattr(fd, TCSAFLUSH, &toptions) < 0) {
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
uint64_t millis() {
	struct timeval time; // Holds the current time in seconds + microsecondsx
	gettimeofday(&time, NULL);
	uint64_t s1 = (uint64_t)(time.tv_sec) * 1000;  // Convert seconds to milliseconds
	uint64_t s2 = (time.tv_usec / 1000);		   // Convert microseconds to milliseconds
	return s1 + s2;
}

// TODO: These are copy pasted from net_util.c. Maybe move these into runtime_util.c
/*
 * Read n bytes from fd into buf; return number of bytes read into buf (deals with interrupts and unbuffered reads)
 * Arguments:
 *    - int fd: file descriptor to read from
 *    - void *buf: pointer to location to copy read bytes into
 *    - size_t n: number of bytes to read
 * Return:
 *    - > 0: number of bytes read into buf
 *    - 0: read EOF on fd
 *    - -1: read errored out
 */
int readn (int fd, void *buf, uint16_t n)
{
	uint16_t n_remain = n;
	uint16_t n_read;
	char *curr = buf;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	while (n_remain > 0) {
		if ((n_read = read(fd, curr, n_remain)) < 0) {
			if (errno == EINTR) { //read interrupted by signal; read again
				n_read = 0;
			} else {
				perror("read");
				log_printf(ERROR, "reading from fd failed");
				return -1;
			}
		} else if (n_read == 0) { //received EOF
			return 0;
		}
		n_remain -= n_read;
		curr += n_read;
	}
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

	return (n - n_remain);
}

/*
 * Read n bytes from buf to fd; return number of bytes written to buf (deals with interrupts and unbuffered writes)
 * Arguments:
 *    - int fd: file descriptor to write to
 *    - void *buf: pointer to location to read from
 *    - size_t n: number of bytes to write
 * Return:
 *    - >= 0: number of bytes written into buf
 *    - -1: write errored out
 */
int writen (int fd, void *buf, uint16_t n)
{
	uint16_t n_remain = n;
	uint16_t n_written;
	char *curr = buf;

	while (n_remain > 0) {
		if ((n_written = write(fd, curr, n_remain)) <= 0) {
			if (n_written < 0 && errno == EINTR) { //write interrupted by signal, write again
				n_written = 0;
			} else {
				perror("write");
				log_printf(ERROR, "writing to fd failed");
				return -1;
			}
		}
		n_remain -= n_written;
		curr += n_written;
	}
	return n;
}

// ************************************ MAIN ****************************************** //

int main() {
	// If SIGINT (Ctrl+C) is received, call sigintHandler to clean up
	signal(SIGINT, stop);
	init();
    log_printf(INFO, "DEV_HANDLER starting with OUTPUT==%s\n", (OUTPUT == USB_DEV) ? "USB_DEV" : "FILE_DEV");
    // Start polling if OUTPUT == USB
    poll_connected_devices();
	return 0;
}
