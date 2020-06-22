#include "fake_device.h"

/* Struct shared between the three threads so they can communicate with each other
 * relayer thread acts as the "control center" and connects/disconnects to shared memory
 *  relayer thread also frees memory when device is disconnected
 */
typedef struct msg_relay {
    pthread_t sender;
    pthread_t receiver;
    pthread_t relayer;
    FILE* read_file;        // File to read from
    FILE* write_file;       // File to write to
    uint8_t start;		    // set by relayer: A flag to tell reader and receiver to start work when set to 1
    dev_id_t dev_id;        // Device ID of this device
    uint8_t got_write;      // set by receiver to tell sender to reply with a DeviceData on written values
    uint8_t got_read;       // set by receiver to tell sender to reply with a DeviceData on read values
    uint8_t got_hb_req;	    // set by receiver: A flag to tell sender to send a HeartBeatResponse
    uint64_t last_sent_hbreq_time;      // set by sender: The last timestamp of a sent HeartBeatRequest. Used to detect timeout
    uint64_t last_received_hbresp_time; // set by receiver: The last timestamp of a received HeartBeatResponse. Used to detect timeout
} msg_relay_t;

/* Returns the number of milliseconds since the Unix Epoch */
int64_t millis() {
	struct timeval time; // Holds the current time in seconds + microsecondsx
	gettimeofday(&time, NULL);
	int64_t s1 = (int64_t)(time.tv_sec) * 1000; // Convert seconds to milliseconds
	int64_t s2 = (time.tv_usec / 1000);			// Convert microseconds to milliseconds
	return s1 + s2;
}

/* Called by relayer to clean up after the device.
 * Releases interface, closes device, cancels threads,
 * disconnects from shared memory, and frees the RELAY struct
 */
void relay_clean_up(msg_relay_t* relay) {
	printf("INFO: Cleaning up threads\n");
	// Cancel the sender and receiver threads when ongoing transfers are completed
	pthread_cancel(relay->sender);
	pthread_cancel(relay->receiver);
	// Sleep to ensure that the sender and receiver threads are cancelled before disconnecting from shared memory / freeing relay
	sleep(1);
	free(relay);
	printf("INFO: Cleaned up threads\n");
	pthread_cancel(pthread_self());
}

/*
 * Attempts to read a message in to_device.txt and parses it into a message_t
 * msg: The message_t* to be populated with the parsed data (if successful)
 * return:  0 if successfully parsed
 *          1 if no message
 *          2 if broken message
 *          3 if incorrect checksum
 */
int receive_message(msg_relay_t* relay, message_t* msg) {
    uint8_t last_byte_read;
    // Try to read the delimiter
    size_t num_bytes_read = fread(&last_byte_read, sizeof(uint8_t), 1, relay->read_file);
    if (num_bytes_read == 0) {
        return 1;
    } else if (last_byte_read != 0) {
        printf("FATAL: Read something besides the delimiter\n");
        return 2;
    }
    // Try to read the length of the cobs encoded message
    num_bytes_read = fread(&last_byte_read, sizeof(uint8_t), 1, relay->read_file);
    if (num_bytes_read == 0) {
        return 1;
    }
    int cobs_len = last_byte_read;
    // Allocate buffer to read message into
    uint8_t data[cobs_len];
    // Read the message
    num_bytes_read = fread(data, sizeof(uint8_t), cobs_len, relay->read_file);
    if (num_bytes_read != cobs_len) {
        printf("FATAL: Couldn't read the full message\n");
        return 2;
    }
    // Parse the message
    if (parse_message(data, msg) != 0) {
        printf("FATAL: Incorrect checksum\n");
        return 3;
    }
    return 0;
}

/*
 * Attempts to serialize and cobs encode a message into to_dev_handler.txt
 * msg: Pointer to the message_t to be serialized, cobs-encoded, and written to the .txt file
 */
void send_message(msg_relay_t* relay, message_t* msg) {
    // Allocate memory for data buffer
    int len = calc_max_cobs_msg_length(msg);
    uint8_t data[len];
    // Fill data buffer with serialized, then encoded message
    len = message_to_bytes(msg, data, len);
    // Write the data buffer to the file
    fwrite(data, sizeof(uint8_t), len, relay->write_file);
}

/*
 *  Thread to continuously send messages to to_dev_handler.txt
 *  Sends DeviceData, HeartBeatRequest, HeartBeatResponse, and Log
 */
void* sender(void* relay_cast) {
    msg_relay_t* relay = (msg_relay_t*) relay_cast;
    printf("INFO: Sender by standby\n");
    // Sleep until relay->start == 1, which is true when relayer sends a SubscriptionResponse
	while (1) {
		if (relay->start == 1) {
			break;
		}
		pthread_testcancel(); // Cancellation point. Relayer may cancel this thread if device timesout
	}
    printf("INFO: Sender starting work\n");
    // Allocate a message to be sent
    message_t* msg = make_empty(MAX_PAYLOAD_SIZE);
    // Continuously send messages as needed
    while (1) {
        if (relay->got_hb_req) {
            // Send a HeartBeatResponse
            message_t* msg = make_heartbeat_response(0);
            send_message(relay, msg);
            destroy_message(msg);
            relay->got_hb_req = 0;
        } else if ((millis() - relay->last_sent_hbreq_time) > HB_REQ_FREQ) {
            // If it's been been too long since the last HeartBeatRequest, send another one
            message_t* msg = make_heartbeat_request(0);
            send_message(relay, msg);
            destroy_message(msg);
            relay->last_sent_hbreq_time = millis();
        } else if (relay->got_write) {
            // TODO: Send DeviceData
            relay->got_write = 0;
        } else if (relay->got_read) {
            // TODO: Send DeviceData
            relay->got_read = 0;
        }
        pthread_testcancel();
    }
}

/*
 *  Thread to continuously read messages from to_device.txt and take action based on the message
 *  Receives DeviceRead, DeviceWrite, HeartBeatRequest, HeartBeatResponse
 */
void* receiver(void* relay_cast) {
    msg_relay_t* relay = (msg_relay_t*) relay_cast;
    printf("INFO: Receiver by standby\n");
    // Sleep until relay->start == 1, which is true when relayer sends a SubscriptionResponse
	while (1) {
		if (relay->start == 1) {
			break;
		}
		pthread_testcancel(); // Cancellation point. Relayer may cancel this thread if device timesout
	}
    printf("INFO: Receiver starting work\n");
    // Allocate message
    message_t* msg = make_empty(MAX_PAYLOAD_SIZE);
    // Continuously read messages and take appropriate action
    while (1) {
        if (receive_message(relay, msg) != 0) {
            continue;
        }
        if (msg->message_id = DeviceRead) {
            // TODO: Send DeviceData
        } else if (msg->message_id == DeviceWrite) {
            // TODO: Change vals and send DeviceData
        } else if (msg->message_id == HeartBeatRequest) {
            relay->got_hb_req = 1;
        } else if (msg->message_id == HeartBeatResponse) {
            relay->last_received_hbresp_time = millis();
        } else {
            printf("FATAL: Got message of unknown type\n");
        }
        pthread_testcancel();
    }
}

/*
 * Thread to make sure sender and receiver and synced
 */
void* relayer(void* relay_cast) {
    msg_relay_t* relay = (msg_relay_t*) relay_cast;

    // Keep reading from file until we get a Ping
    printf("INFO: Relayer trying to read a ping\n");
    uint8_t last_byte_read = -1;
    while (last_byte_read != 0x00) {
        fread(&last_byte_read, sizeof(uint8_t), 1, relay->read_file);
    }
    printf("INFO: Relayer found start of message\n");
    uint8_t cobs_len;
    fread(&cobs_len, sizeof(uint8_t), 1, relay->read_file); // Read cobs length
    uint8_t data[cobs_len];
    fread(data, sizeof(uint8_t), cobs_len, relay->read_file); // Read the message
    printf("INFO: Relayer parsing message\n");
    message_t* msg = make_empty(MAX_PAYLOAD_SIZE);
    if (parse_message(data, msg) != 0) {
        printf("ERROR: First message read had incorrect checksum\n");
        relay_clean_up(relay);
        return NULL;
    }
    if (msg->message_id != Ping) {
        printf("ERROR: First message was not a Ping\n");
        relay_clean_up(relay);
        return NULL;
    }

    // Send a SubscriptionResponse
    printf("INFO: Relayer got a ping and sending a SubscriptionResponse\n");
    message_t* sub_response = make_subscription_response(&relay->dev_id, NULL, 0, 0);
    send_message(relay, sub_response);

    // Signal sender and receiver to start work
    relay->start = 1;

    // Make sure DEV_HANDLER responds to our HeartBeatRequest in a timely manner
    printf("INFO: Relayer monitoring heartbeats\n");
    while (1) {
        uint8_t pending_request = relay->last_sent_hbreq_time > relay->last_received_hbresp_time;
        if (pending_request && ((millis() - relay->last_sent_hbreq_time) > TIMEOUT)) {
            printf("ERROR: DEV_HANDLER didn't respond to our HeartBeatRequest in time!\n");
            relay_clean_up(relay);
            return NULL;
        }
    }
}

void init() {
    // Allocate relay object
    printf("INFO: Making relay struct\n");
    msg_relay_t* relay = malloc(sizeof(msg_relay_t));
    // Open the file to read from
    relay->read_file = fopen(TO_DEVICE, "w+");
    if (relay->read_file == NULL) {
        printf("ERROR: Couldn't open TO_DEVICE file with error code %d\n", errno);
    }
    // Open the file to write to
    relay->write_file = fopen(TO_DEV_HANDLER, "w+");
    if (relay->write_file == NULL) {
        printf("ERROR: Couldn't open TO_DEV_HANDLER file\n");
    }
    // Don't start sending and receiving just yet
    relay->start = 0;
    // Set the device id
    relay->dev_id.type = TYPE;
    relay->dev_id.year = YEAR;
    relay->dev_id.uid = UID;
    // Open threads for sender, receiver, and relayer
	if (pthread_create(&relay->sender, NULL, sender, relay) != 0) {
        printf("ERROR: Couldn't create sender\n");
    }
	if (pthread_create(&relay->receiver, NULL, receiver, relay) != 0) {
        printf("ERROR: Couldn't create receiver\n");
    }
	if (pthread_create(&relay->relayer, NULL, relayer, relay) != 0) {
        printf("ERROR: Couldn't create relayer\n");
    }
    printf("INFO: Fake device powered on\n");
    pthread_join(relay->relayer, NULL); // Let the threads do work
}

int main() {
    init();
}
