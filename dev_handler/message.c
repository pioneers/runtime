/*
Constructs, encodes, and decodes the appropriate messages asked for by dev_handler.c
Previously hibikeMessage.py

Linux: json-c:      sudo apt-get install -y libjson-c-dev
       compile:     gcc -I/usr/include/json-c -L/usr/lib message.c -o message -ljson-c
       // Note that -ljson-c comes AFTER source files: https://stackoverflow.com/questions/31041272/undefined-reference-error-while-using-json-c
Mac:
       compile:     gcc -I/usr/local/include/json-c -L/usr/local/lib/ message.c -o message -ljson-c

*/
#include "message.h"

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

/***************************************************
*               MESSAGE CONSTRUCTORS               *
* Constructs message_t to be encoded and sent      *
***************************************************/
message_t* make_empty(ssize_t payload_size) {
    message_t* msg = malloc(sizeof(message_t));
    msg->payload = malloc(payload_size);
    msg->payload_length = 0;
    msg->max_payload_length = payload_size;
    return msg;
}

message_t* make_ping() {
    message_t* ping = malloc(sizeof(message_t));
    ping->message_id = Ping;
    ping->payload = NULL;
    ping->payload_length = 0;
    ping->max_payload_length = 0;
    return ping;
}

message_t* make_disable() {
    message_t* disable = malloc(sizeof(message_t));
    disable->message_id = Disable;
    disable->payload = NULL;
    disable->payload_length = 0;
    disable->max_payload_length = 0;
    return disable;
}

message_t* make_heartbeat_request(char heartbeat_id) {
    message_t* heartbeat_request = malloc(sizeof(message_t));
    heartbeat_request->message_id = HeartBeatRequest;
    heartbeat_request->payload = malloc(1); // Payload is 8 bits == 1 byte
    heartbeat_request->payload[0] = heartbeat_id ? heartbeat_id : 0;
    // Note that the content of payload is unused at the moment
    // May be used in the future to calculate latency
    heartbeat_request->payload_length = 1;
    heartbeat_request->max_payload_length = 1;
    return heartbeat_request;
}

message_t* make_heartbeat_response(char heartbeat_id) {
    message_t* heartbeat_response = malloc(sizeof(message_t));
    heartbeat_response->message_id = HeartBeatResponse;
    heartbeat_response->payload = malloc(1); // Payload is 8 bits == 1 byte
    heartbeat_response->payload[0] = heartbeat_id ? heartbeat_id : 0;
    // Note that the content of payload is unused at the moment
    // May be used in the future to calculate latency
    heartbeat_response->payload_length = 1;
    heartbeat_response->max_payload_length = 1;
    return heartbeat_response;
}

/*
 * Constructs a message given DEVICE_ID, array of param names PARAM_NAMES of length LEN,
 * and a DELAY in milliseconds
 * Note: Payload a 32-bit bit mask followed by the 16-bit delay
 *          --> 48-bit == 6-byte payload
*/
message_t* make_subscription_request(dev_id_t* device_id, char* param_names[], uint8_t len, uint16_t delay) {
    message_t* sub_request = malloc(sizeof(message_t));
    sub_request->message_id = SubscriptionRequest;
    sub_request->payload = malloc(BITMAP_SIZE + DELAY_SIZE);
    sub_request->payload_length = 0;
    sub_request->max_payload_length = BITMAP_SIZE + DELAY_SIZE;
    // Fill in 32-bit params mask
    uint32_t mask = encode_params(device_id->type, param_names, len);
    int status = 0;
    status += append_payload(sub_request, (uint8_t*) &mask, BITMAP_SIZE);
    status += append_payload(sub_request, (uint8_t*) &delay, DELAY_SIZE);
    return (status == 0) ? sub_request : NULL;
}

/*
 * Constructs a subscription response given DEVICE_ID, array of param names PARAM_NAMES of length LEN,
 * a DELAY in milliseconds
 * Payload: 32-bit params + 16-bit delay + 88-bit dev_id
 *          --> 136-bits == 17 bytes
*/
/*
message_t* make_subscription_response(dev_id_t* device_id, char* param_names[], uint8_t len, uint16_t delay) {
    message_t* sub_response = malloc(sizeof(message_t));
    sub_response->message_id = SubscriptionResponse;
    sub_response->payload = malloc(BITMAP_SIZE + DELAY_SIZE + DEVICE_ID_SIZE);
    sub_response->payload_length = 0;
    sub_response->max_payload_length = BITMAP_SIZE + DELAY_SIZE + DEVICE_ID_SIZE;
    uint32_t mask = encode_params(device_id->type, param_names, len);
    int status = 0;
    status += append_payload(sub_response, (uint8_t*) &mask, BITMAP_SIZE);
    status += append_payload(sub_response, (uint8_t*) &delay, DELAY_SIZE);
    status += append_payload(sub_response, (uint8_t*) &(device_id->uid), DEVICE_ID_SIZE);
    return (status == 0) ? sub_response : NULL;
}
*/

/*
 * Constructs a DeviceRead message, given DEVICE_ID and array of param names PARAM_NAMES of length Len.
 * Requests the device to send data about the PARAMS
 * Payload: 32-bit param mask == 8 bytes
*/
message_t* make_device_read(dev_id_t* device_id, char* param_names[], uint8_t len) {
    message_t* dev_read = malloc(sizeof(message_t));
    dev_read->message_id = DeviceRead;
    dev_read->payload = malloc(BITMAP_SIZE);
    dev_read->payload_length = 0;
    dev_read->max_payload_length = BITMAP_SIZE;
    uint32_t mask = encode_params(device_id->type, param_names, len);
    int status = 0;
    status += append_payload(dev_read, (uint8_t*) &mask, BITMAP_SIZE);
    return (status == 0) ? dev_read : NULL;
}

/*
 * A message to write data to the specified writable parameters of a device
 * The device is expected to respond with a DeviceData message confirming the new data of the writable parameters.
 * device_id: The id of the device to write data to
 * param_bitmap: The 32-bit param bitmap indicating which parameters will be written to
 * param_values: An array of the parameter values. If i-th bit in the bitmap is on, its value is in the i-th index.
 *
 * Payload: 32-bit param mask, each of the param_values specified (number of bytes depends on the parameter type)
 * Direction: dev_handler --> device
 */
message_t* make_device_write(dev_id_t* device_id, uint32_t param_bitmap, param_val_t param_values[]) {
    message_t* dev_write = malloc(sizeof(message_t));
    dev_write->message_id = DeviceWrite;
    dev_write->payload_length = 0;
    dev_write->max_payload_length = device_data_payload_size(device_id->type, param_bitmap);
    dev_write->payload = malloc(dev_write->max_payload_length);
    int status = 0;
    // Append the mask
    status += append_payload(dev_write, (uint8_t*) &param_bitmap, BITMAP_SIZE);
    // Build the payload with the values
    device_t* dev = get_device(device_id->type);
    for (int i = 0; i < MAX_PARAMS; i++) {
        // If the parameter is off in the bitmap, skip it
        if (((1 << i) & param_bitmap) == 0) {
            continue;
        }
        char* param_type = dev->params[i].type;
        if (strcmp(param_type, "int") == 0) {
            status += append_payload(dev_write, (uint8_t*) &(param_values[i].p_i), sizeof(int));
        } else if (strcmp(param_type, "float") == 0) {
            status += append_payload(dev_write, (uint8_t*) &(param_values[i].p_f), sizeof(float));
        } else if (strcmp(param_type, "bool") == 0) { // Boolean
            status += append_payload(dev_write, (uint8_t*) &(param_values[i].p_b), sizeof(uint8_t));
        }
    }
    return (status == 0) ? dev_write : NULL;
}

/*
 * Returns a message with a 32-bit param mask and thirty-two 32-bit values
 * Logic is the same as make_device_write.
 */
/*
message_t* make_device_data(dev_id_t* device_id, uint32_t param_bitmap, param_val_t param_values[]) {
    message_t* dev_data = make_device_write(device_id, param_bitmap, param_values);
    dev_data->message_id = DeviceData;
    return dev_data;
}
*/

/*
 * Returns a new message with DATA as its payload
 */
/*
message_t* make_log(char* data) {
    if ((strlen(data) + 1) > (MAX_PAYLOAD_SIZE * sizeof(char))) {
        printf("Error in making message: DATA IS TOO LONG.\n");
        return NULL;
    }
    message_t* log = malloc(sizeof(message_t));
    log->message_id = Log;
    log->payload = malloc(MAX_PAYLOAD_SIZE);
    log->payload_length = 0;
    log->max_payload_length = MAX_PAYLOAD_SIZE;
    strcpy((char*) log->payload, data);
    log->payload_length = (uint8_t) strlen(data) + 1;
    return log;
}

message_t* make_error(uint8_t error_code) {
    message_t* error = malloc(sizeof(message_t));
    error->message_id = Error;
    error->payload = malloc(1);
    error->payload[0] = error_code;
    error->payload_length = 1;
    error->max_payload_length = 1;
    return error;
}
*/

void destroy_message(message_t* message) {
    free(message->payload);
    free(message);
}

/*
 * Appends DATA with length LENGTH to the end of the payload of MSG
 * msg->payload is an array of 8-bit integers
 * msg->payload_length is the current length of the data in the payload
 * msg->max_payload_length is the actual length of the payload (all of which may not be used)
 * Returns -1 if payload_length > max_payload_length (overwrite)
 * Returns 0 on success
 * ---This is more or less the same as Messenger::append_payload in Messenger.cpp
*/
int append_payload(message_t *msg, uint8_t *data, uint8_t length)
{
	memcpy(&(msg->payload[msg->payload_length]), data, length);
	msg->payload_length += length;
	return (msg->payload_length > msg->max_payload_length) ? -1 : 0;
}

/*
 * Compute the checksum of byte-array DATA of length LEN
*/
uint8_t checksum(uint8_t* data, int len) {
    uint8_t chk = data[0];
    for (int i = 0; i < len; i++) {
        chk ^= data[i];
    }
    return chk;
}

/***************************************************
*               COBS ENCODE/DECODE
* Implementation copied from Messenger.cpp in lowcar
***************************************************/
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
Encodes src into dst and returns the size of dst. src nust not overlap dst.
Replace all zero bytes with nonzero bytes describing the location of the next zero
For detailed example, view https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing#Encoding_examples
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
 * Decodes src into dst and returns the size of dst. src may overlap dst.
 Revert back into original non-cobs_encoded data
 For example, view https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing#Encoding_examples
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

/*
 * Given string array PARAMS of length LEN consisting of params of DEVICE_UID,
 * Generate a bit-mask of 1s (if param is in PARAMS), 0 otherwise
 * Ex: encode_params(0, ["switch2", "switch1"], 2)
 *  Switch2 is param 2, switch1 is param 1
 *  Return 110  (switch2 on, switch1 on, switch0 off)
*/
uint32_t encode_params(uint16_t device_type, char** params, uint8_t len) {
    uint8_t param_nums[len]; // [1, 9, 2] -> [0001, 1001, 0010]
    device_t* dev = get_device(device_type);
    int num_params_in_dev = dev->num_params;
    // Iterate through PARAMS and find its corresponding number in the official list
    for (int i = 0; i < len; i++) {
        // Iterate through official list of params to find the param number
        for (int j = 0; j < num_params_in_dev; j++) {
            if (strcmp(dev->params[j].name, params[i]) == 0) { // Returns 0 if they're equivalent
                param_nums[i] = j; // DEVICES[device_type]->params[j].number;
                break;
            }
        }
    }
    // Generate mask by OR-ing numbers in param_nums
    uint32_t mask = 0;
    for (int i = 0; i < len; i++) {
        mask = mask | (1 << param_nums[i]);
    }
    return mask;
}

/*
 * Given a device_type and a mask, return an array of param names
 * Encode 1, 9, 2 --> 1^9^2 = 10
 * Decode 10 --> 1, 9, 2
*/
/* To be determined; not yet sure how we want to return an array of strings
char** decode_params(uint16_t device_type, uint32_t mask) {
    // Iterate through MASK to find the name of the parameter
    int len = 0;
    int copy = mask; // 111 --> ["switch0", "switch1", "switch2"]
    // Count the number of bits that are on (number of params encoded)
    while (copy != 0) {
        len += copy & 1;
        copy >>= 1;
    }
    char* param_names[len];
    int i = 0;
    // Get the names of the params
    for (int j = 0; j < 32; j++) {
        if (mask & (1 << j)) { // If jth bit is on (from the right)
            param_names[i] = DEVICES[device_type]->params[j].name;
            i++;
        }
    }
    return param_names;
}
*/

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
    uint8_t data[MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length + CHECKSUM_SIZE];
    data[0] = msg->message_id;
    data[1] = msg->payload_length;
    for (int i = 0; i < msg->payload_length; i++) {
        data[i + MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE] = msg->payload[i];
    }
    data[MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length] = checksum(data, MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length);

    cobs_encoded[0] = 0x00;
    int cobs_len = cobs_encode(&cobs_encoded[2], data, 3 + msg->payload_length);
    cobs_encoded[1] = cobs_len;
    return DELIMITER_SIZE + COBS_LENGTH_SIZE + cobs_len;
}

int parse_message(uint8_t data[], message_t* msg_to_fill) {
    uint8_t* decoded = malloc(MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg_to_fill->payload_length + CHECKSUM_SIZE);
    cobs_decode(decoded, &data[2], data[1]);
    msg_to_fill->message_id = decoded[0];
    msg_to_fill->payload_length = decoded[1];
    msg_to_fill->max_payload_length = decoded[1];
    for (int i = 0; i < msg_to_fill->payload_length; i++) {
        msg_to_fill->payload[i] = decoded[MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + i];
    }
    char expected_checksum = decoded[MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg_to_fill->payload_length];
    char received_checksum = checksum(decoded, MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg_to_fill->payload_length);
    free(decoded);
    return (expected_checksum != received_checksum) ? 1 : 0;
}

void parse_device_data(uint16_t dev_type, message_t* dev_data, param_val_t* vals[]) {
    device_t* dev = get_device(dev_type);
    // Bitmap is stored in the first 32 bits of the payload
    uint32_t bitmap = ((uint32_t*) dev_data->payload)[0];
    /* Iterate through device's parameters. If bit is off, continue
     * If bit is on, determine how much to read from the payload then put it in VALS in the appropriate field */
    uint8_t* payload_ptr = &(dev_data->payload[BITMAP_SIZE]); // Start the pointer at the beginning of the values (skip the bitmap)
    int vals_idx = 0;
    for (int i = 0; i < dev->num_params; i++) {
        // If bit is off, parameter is not included in the payload
        if (((1 << i) & bitmap) == 0) {
            continue;
        }
        if (strcmp(dev->params[i].type, "int") == 0) {
            vals[vals_idx]->p_i = ((int*) payload_ptr)[0];
            payload_ptr += sizeof(int) / sizeof(uint8_t);
        } else if (strcmp(dev->params[i].type, "float") == 0) {
            vals[vals_idx]->p_f = ((float*) payload_ptr)[0];
            payload_ptr += sizeof(float) / sizeof(uint8_t);
        } else if (strcmp(dev->params[i].type, "bool") == 0) {
            vals[vals_idx]->p_b = payload_ptr[0];
            payload_ptr += sizeof(uint8_t) / sizeof(uint8_t);
        }
        vals_idx++;
    }
}

/*
param_val_t** make_empty_param_values(uint32_t param_bitmap) {
    // Calculate the length of the array to return
    int len = 0;
    while (param_bitmap != 0) {
        len += (param_bitmap & 1);
        param_bitmap >>= 1;
    }
    param_val_t** result = malloc(len * sizeof(param_val_t*));
    // Allocate memory for each index
    for (int i = 0; i < len; i++) {
        result[i] = malloc(sizeof(param_val_t));
    }
    return result;
}

void destroy_param_values(uint32_t param_bitmap, param_val_t* vals[]) {
    // Calculate the length of the VALS
    int len = 0;
    while (param_bitmap != 0) {
        len += (param_bitmap & 1);
        param_bitmap >>= 1;
    }
    // Free each index
    for (int i = 0; i < len; i++) {
        free(vals[i]);
    }
    // Free the array itself
    free(vals);
}
*/
