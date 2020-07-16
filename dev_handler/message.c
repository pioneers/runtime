#include "message.h"

/* Prints the byte array DATA of length LEN in hex */
void print_bytes(uint8_t* data, int len) {
	printf("0x");
	for (int i = 0; i < len; i++) {
		printf("%02X ", data[i]);
	}
	printf("\n");
}

// ************************************ PRIVATE FUNCTIONS ****************************************** //

/*
 * Private utility function to calculate the size of the payload needed
 * for a DeviceWrite/DeviceData message.
 * device_type: The type of device (Ex: 2 for Potentiometer)
 * param_bitmap: 32-bit param bit map. The i-th bit indicates whether param i will be transmitted in the message
 * return: The size of the payload
 */
static ssize_t device_data_payload_size(uint16_t device_type, uint32_t param_bitmap) {
    ssize_t result = BITMAP_SIZE;
    device_t* dev = get_device(device_type);
    // Loop through each of the device's parameters and add the size of the parameter
    for (int i = 0; i < dev->num_params; i++) {
        // Ignore parameter i if the i-th bit is off
        if (((1 << i) & param_bitmap) == 0) {
            continue;
        }
        char* type = dev->params[i].type;
        if (strcmp(type, "int") == 0) {
            result += sizeof(int);
        } else if (strcmp(type, "float") == 0) {
            result += sizeof(float);
        } else {
            result += sizeof(uint8_t);
        }
    }
    return result;
}

/*
 * Appends data to the end of a message's payload
 * msg: The message whose payload is to be appended to
 * data: The data to be appended to the payload
 * length: The length of data in bytes
 * returns: 0 if successful. -1 otherwise due to overwriting
 * side-effect: increments msg->payload_length by length
 */
int append_payload(message_t *msg, uint8_t *data, uint8_t length)
{
	memcpy(&(msg->payload[msg->payload_length]), data, length);
	msg->payload_length += length;
	return (msg->payload_length > msg->max_payload_length) ? -1 : 0;
}


/*
 * Computes the checksum of data
 * data: An array
 * len: the length of data
 * returns: The checksum
 */
uint8_t checksum(uint8_t* data, int len) {
    uint8_t chk = data[0];
    for (int i = 1; i < len; i++) {
        chk ^= data[i];
    }
    return chk;
}

/*
 * A macro to help with cobs_encode
 */
#define finish_block() {		\
	*block_len_loc = block_len; \
	block_len_loc = dst++;      \
	out_len++;                  \
	block_len = 0x01;           \
}

/*
 * Cobs encodes data into a buffer
 * src: The data to be encoded
 * dst: The buffer to write the encoded data to
 * src_len: The size of the source data
 * return: The size of the encoded data
 */
ssize_t cobs_encode(uint8_t *dst, const uint8_t *src, ssize_t src_len) {
	const uint8_t *end = src + src_len;
	uint8_t *block_len_loc = dst++;
	uint8_t block_len = 0x01;
	ssize_t out_len = 0;

	while (src < end) {
		if (*src == 0) {
			finish_block();
		} else {
			*dst++ = *src;
			block_len++;
			out_len++;
			if (block_len == 0xFF) {
				finish_block();
			}
		}
		src++;
	}
	finish_block();

	return out_len;
}

/*
 * Cobs decodes data into a buffer
 * src: The data to be decoded
 * dst: The buffer to write the decoded data to
 * src_len: The size of the source data
 * return: The size of the decoded data
 */
ssize_t cobs_decode(uint8_t *dst, const uint8_t *src, ssize_t src_len) {
	const uint8_t *end = src + src_len;
	ssize_t out_len = 0;

	while (src < end) {
		uint8_t code = *src++;
		for (uint8_t i = 1; i < code; i++) {
			*dst++ = *src++;
			out_len++;
			if (src > end) { // Bad packet
				return 0;
			}
		}
		if (code < 0xFF && src != end) {
			*dst++ = 0;
			out_len++;
		}
	}
	return out_len;
}

// ************************************ PUBLIC FUNCTIONS ****************************************** //

/***************************************************
*               MESSAGE CONSTRUCTORS               *
* Constructs message_t to be encoded and sent      *
***************************************************/
message_t* make_empty(ssize_t payload_size) {
    message_t* msg = malloc(sizeof(message_t));
    msg->message_id = 0x0;
    msg->payload = malloc(payload_size);
    msg->payload_length = 0;
    msg->max_payload_length = payload_size;
    return msg;
}

message_t* make_ping() {
    message_t* ping = malloc(sizeof(message_t));
    ping->message_id = PING;
    ping->payload = NULL;
    ping->payload_length = 0;
    ping->max_payload_length = 0;
    return ping;
}

/*
 * A message to request parameter data from a device at an interval
 * dev_type: The type of device. Used to verify params are write-able
 * pmap: 32-bit param bitmap indicating which params should be subscribed to
 * interval: The number of milliseconds to wait between each DEVICE_DATA
 *
 * Payload: 32-bit param bitmap, 16-bit interval
 */
message_t* make_subscription_request(uint16_t dev_type, uint32_t pmap, uint16_t interval) {
	device_t* dev = get_device(dev_type);
	// Don't read from non-existent params
	pmap &= ((uint32_t) -1) >> (MAX_PARAMS - dev->num_params);	// Set non-existent params to 0
	// Set non-read-able params to 0
	for (int i = 0; i < MAX_PARAMS; i++) {
		if (dev->params[i].read == 0) {
			pmap &= ~(1 << i);	// Set bit i to 0
		}
	}
    message_t* sub_request = malloc(sizeof(message_t));
    sub_request->message_id = SUBSCRIPTION_REQUEST;
    sub_request->payload = malloc(BITMAP_SIZE + INTERVAL_SIZE);
    sub_request->payload_length = 0;
    sub_request->max_payload_length = BITMAP_SIZE + INTERVAL_SIZE;

    int status = 0;
    status += append_payload(sub_request, (uint8_t*) &pmap, BITMAP_SIZE);
    status += append_payload(sub_request, (uint8_t*) &interval, INTERVAL_SIZE);
    return (status == 0) ? sub_request : NULL;
}

/*
 * A message to write data to the specified writable parameters of a device
 * dev_type: The type of device. Used to verify params are write-able
 * pmap: The 32-bit param bitmap indicating which parameters will be written to
 * param_values: An array of the parameter values. If i-th bit in the bitmap is on, its value is in the i-th index.
 *
 * Payload: 32-bit param mask, each of the param_values specified (number of bytes depends on the parameter type)
 */
message_t* make_device_write(uint16_t dev_type, uint32_t pmap, param_val_t param_values[]) {
	device_t* dev = get_device(dev_type);
	// Don't write to non-existent params
	pmap &= ((uint32_t) -1) >> (MAX_PARAMS - dev->num_params);	// Set non-existent params to 0
	// Set non-write-able params to 0
	for (int i = 0; i < MAX_PARAMS; i++) {
		if (dev->params[i].write == 0) {
			pmap &= ~(1 << i);	// Set bit i to 0
		}
	}
	// Build the message
    message_t* dev_write = malloc(sizeof(message_t));
    dev_write->message_id = DEVICE_WRITE;
    dev_write->payload_length = 0;
    dev_write->max_payload_length = device_data_payload_size(dev_type, pmap);
    dev_write->payload = malloc(dev_write->max_payload_length);
    int status = 0;
    // Append the param bitmap
    status += append_payload(dev_write, (uint8_t*) &pmap, BITMAP_SIZE);
    for (int i = 0; i < MAX_PARAMS; i++) {
        // If the parameter is off in the bitmap, skip it
        if (((1 << i) & pmap) == 0) {
            continue;
        }
		// Determine the size of the parameter and append it accordingly
        char* param_type = dev->params[i].type;
        if (strcmp(param_type, "int") == 0) {
            status += append_payload(dev_write, (uint8_t*) &(param_values[i].p_i), sizeof(int32_t));
        } else if (strcmp(param_type, "float") == 0) {
            status += append_payload(dev_write, (uint8_t*) &(param_values[i].p_f), sizeof(float));
        } else if (strcmp(param_type, "bool") == 0) { // Boolean
            status += append_payload(dev_write, (uint8_t*) &(param_values[i].p_b), sizeof(uint8_t));
        }
    }
    return (status == 0) ? dev_write : NULL;
}

void destroy_message(message_t* message) {
    free(message->payload);
    free(message);
}

/******************************************************************************************
 *                          Serializing and Parsing Messages                              *
 ******************************************************************************************/

int calc_max_cobs_msg_length(message_t* msg){
  int required_packet_length = MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length + CHECKSUM_SIZE;
  // Cobs encoding a length N message adds overhead of at most ceil(N/254)
  int cobs_length = required_packet_length + (required_packet_length / 254) + 1;
  /* Add 2 additional bytes to the buffer for use in message_to_bytes()
   * 0th byte will be 0x0 indicating the start of a packet.
   * 1st byte will hold the actual length from cobs encoding */
  return DELIMITER_SIZE + COBS_LENGTH_SIZE + cobs_length;
}


int message_to_bytes(message_t* msg, uint8_t cobs_encoded[], int len) {
    // Initialize the byte array. See page 8 of book.pdf
    // 1 byte for messageID, 1 byte for payload length, the payload itself, and 1 byte for the checksum
    // + cobe encoding overhead; Source: https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
    int required_length = calc_max_cobs_msg_length(msg);
    if (len < required_length) {
        return -1;
    }
    uint8_t* data = malloc(MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length + CHECKSUM_SIZE);
    data[0] = msg->message_id;

    data[1] = msg->payload_length;
	//log_printf(DEBUG, "Sending message id 0x%02X\n", data[0]);
	//log_printf(DEBUG, "Sending payload length 0x%02X\n", data[1]);
    for (int i = 0; i < msg->payload_length; i++) {
        data[i + MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE] = msg->payload[i];
    }
    data[MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length] = checksum(data, MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length);
	//log_printf(DEBUG, "Sending checksum 0x%02X\n", data[MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length]);
    cobs_encoded[0] = 0x00;
    int cobs_len = cobs_encode(&cobs_encoded[2], data, MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length + CHECKSUM_SIZE);
    cobs_encoded[1] = cobs_len;
	free(data);
    return DELIMITER_SIZE + COBS_LENGTH_SIZE + cobs_len;
}

int parse_message(uint8_t data[], message_t* msg_to_fill) {
    uint8_t* decoded = malloc(MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg_to_fill->max_payload_length + CHECKSUM_SIZE);
    // printf("Cobs len: %d\n", data[1]);
    int ret = cobs_decode(decoded, &data[2], data[1]);
    // printf("Decoded message: ");
    // print_bytes(decoded, ret);
    msg_to_fill->message_id = decoded[0];
    // printf("INFO: Received message id 0x%X\n", decoded[0]);
    msg_to_fill->payload_length = 0;
    msg_to_fill->max_payload_length = decoded[1];
	ret = append_payload(msg_to_fill, &decoded[MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE], decoded[1]);
	if (ret != 0) {
		printf("ERROR: Overwrote to payload\n");
		return 2;
	}
	//printf("Received payload length %d\n", msg_to_fill->payload_length);
    char expected_checksum = checksum(decoded, MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg_to_fill->payload_length);
    char received_checksum = decoded[MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg_to_fill->payload_length];
	if (expected_checksum != received_checksum) {
		printf("Expected checksum 0x%02X. Received 0x%02X\n", expected_checksum, received_checksum);
	}
    free(decoded);
    return (expected_checksum != received_checksum) ? 1 : 0;
}

void parse_device_data(uint16_t dev_type, message_t* dev_data, param_val_t vals[]) {
    device_t* dev = get_device(dev_type);
    // Bitmap is stored in the first 32 bits of the payload
    uint32_t bitmap = ((uint32_t*) dev_data->payload)[0];
    /* Iterate through device's parameters. If bit is off, continue
     * If bit is on, determine how much to read from the payload then put it in VALS in the appropriate field */
    uint8_t* payload_ptr = &(dev_data->payload[BITMAP_SIZE]); // Start the pointer at the beginning of the values (skip the bitmap)
	printf("%s: ", dev->name);
    for (int i = 0; i < MAX_PARAMS; i++) {
        // If bit is off, parameter is not included in the payload
        if (((1 << i) & bitmap) == 0) {
            continue;
        }
        if (strcmp(dev->params[i].type, "int") == 0) {
            vals[i].p_i = ((int32_t*) payload_ptr)[0];
			printf("(%s) %d; ", dev->params[i].name, vals[i].p_i);
            payload_ptr += sizeof(int32_t) / sizeof(uint8_t);
        } else if (strcmp(dev->params[i].type, "float") == 0) {
            vals[i].p_f = ((float*) payload_ptr)[0];
			printf("(%s) %f; ", dev->params[i].name, vals[i].p_f);
            payload_ptr += sizeof(float) / sizeof(uint8_t);
        } else if (strcmp(dev->params[i].type, "bool") == 0) {
            vals[i].p_b = payload_ptr[0];
			printf("(%s) %d; ", dev->params[i].name, vals[i].p_b);
            payload_ptr += sizeof(uint8_t) / sizeof(uint8_t);
        }
    }
	printf("\n");
}
