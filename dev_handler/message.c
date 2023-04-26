#include "message.h"

// ******************************** Utility ********************************* //

void print_bytes(uint8_t* data, size_t len) {
    printf("0x");
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

// *************************** PRIVATE FUNCTIONS **************************** //

/**
 * Private utility function to calculate the size of the payload needed
 * for a DEVICE_WRITE message.
 * Arguments:
 *    device_type: The type of device (refer to runtime_util)
 *    param_bitmap: A bitmap, the i-th bit indicates whether param i will be transmitted in the message
 * Returns:
 *    The size of the payload
 */
static size_t device_write_payload_size(uint8_t device_type, uint32_t param_bitmap) {
    size_t result = BITMAP_SIZE;
    device_t* dev = get_device(device_type);
    // Loop through each of the device's parameters and add the size of the parameter
    for (int i = 0; ((param_bitmap >> i) > 0) && (i < MAX_PARAMS); i++) {
        // Include parameter i if the i-th bit is on
        if ((1 << i) & param_bitmap) {
            switch (dev->params[i].type) {
                case INT:
                    result += sizeof(int32_t);
                    break;
                case FLOAT:
                    result += sizeof(float);
                    break;
                case BOOL:
                    result += sizeof(uint8_t);
                    break;
            }
        }
    }
    return result;
}

/**
 * Appends data to the end of a message's payload
 * Increments msg->payload_length accordingly
 * Arguments:
 *    msg: The message whose payload is to be appended to
 *    data: The data to be appended to MSG's payload
 *    len: The size of data
 * Returns:
 *    0 if successful
 *    -1 if max_payload_length is too small
 */
static int append_payload(message_t* msg, uint8_t* data, size_t len) {
    memcpy(&(msg->payload[msg->payload_length]), data, len);
    msg->payload_length += len;
    return (msg->payload_length > msg->max_payload_length) ? -1 : 0;
}

/**
 * Computes the checksum of input byte array by XOR-ing each byte
 * Arguments:
 *    data: A byte array whose checksum will be computed
 *    len: the size of data
 * Returns:
 *    The checksum of DATA
 */
static uint8_t checksum(uint8_t* data, size_t len) {
    uint8_t chk = data[0];
    for (int i = 1; i < len; i++) {
        chk ^= data[i];
    }
    return chk;
}

/**
 * A macro to help with cobs_encode
 */
#define finish_block()                  \
    {                                   \
        block[0] = (uint8_t) block_len; \
        block = dst;                    \
        dst++;                          \
        dst_len++;                      \
        block_len = 1;                  \
    };

/**
 * Cobs encodes a byte array into a buffer
 * Arguments:
 *    src: The byte array to be encoded
 *    dst: The buffer to write the encoded data into
 *    src_len: The size of SRC
 * Returns:
 *    The size of the encoded data, DST
 */
static ssize_t cobs_encode(uint8_t* dst, const uint8_t* src, size_t src_len) {
    const uint8_t* end = src + src_len;
    uint8_t* block = dst;  // Advancing pointer to start of each block
    dst++;
    size_t block_len = 1;
    ssize_t dst_len = 0;

    /* Build the DST array in "blocks", copying bytes one-by-one from SRC
     * until a block ends.
     * A block ends when
     * 1) Encountering a 0x00 byte in the source array,
     * 2) Reaching the max length of 256, or
     * 3) Source array is fully processed
     * When a block ends, insert the block length in DST at the beginning of
     * that block, then start a new block if there are still bytes in SRC to process
     */
    while (src < end) {
        if (*src == 0) {
            finish_block();
        } else {
            // Copy non-zero byte over without processing
            *dst = *src;
            dst++;
            block_len++;
            dst_len++;
            if (block_len == 0xFF) {
                // Reached max block length
                finish_block();
            }
        }
        src++;
    }
    finish_block();
    return dst_len;
}

/**
 * Cobs decodes a byte array into a buffer
 * Arguments:
 *    src: The byte array to be decoded
 *    dst: The buffer to write the decoded data into
 *    src_len: The size of SRC
 * Returns:
 *    The size of the decoded data, DST
 */
static ssize_t cobs_decode(uint8_t* dst, const uint8_t* src, size_t src_len) {
    // Pointer to end of source array
    const uint8_t* end = src + src_len;
    // Size counter of decoded array to return
    ssize_t out_len = 0;

    while (src < end) {
        int num_bytes_to_copy = *src;
        src++;
        for (int i = 1; i < num_bytes_to_copy; i++) {
            *dst = *src;
            dst++;
            src++;
            out_len++;
            if (src > end) {  // Bad packet
                return 0;
            }
        }
        if (src != end) {
            // Start decoding a new block, putting back the zero
            *dst++ = 0;
            out_len++;
        }
    }
    return out_len;
}

// ************************* MESSAGE CONSTRUCTORS *************************** //

message_t* make_empty(ssize_t payload_size) {
    message_t* msg = malloc(sizeof(message_t));
    if (msg == NULL) {
        log_printf(FATAL, "make_empty: Failed to malloc");
        exit(1);
    }
    msg->message_id = NOP;
    msg->payload = malloc(payload_size);
    if (msg->payload == NULL) {
        log_printf(FATAL, "make_empty: Failed to malloc");
        exit(1);
    }
    msg->payload_length = 0;
    msg->max_payload_length = payload_size;
    return msg;
}

message_t* make_ping() {
    message_t* ping = malloc(sizeof(message_t));
    if (ping == NULL) {
        log_printf(FATAL, "make_ping: Failed to malloc");
        exit(1);
    }
    ping->message_id = DEVICE_PING;
    ping->payload = NULL;
    ping->payload_length = 0;
    ping->max_payload_length = 0;
    return ping;
}

message_t* make_device_write(uint8_t dev_type, uint32_t pmap, param_val_t param_values[]) {
    device_t* dev = get_device(dev_type);
    // Don't write to non-existent params
    pmap &= ((uint32_t) -1) >> (MAX_PARAMS - dev->num_params);  // Set non-existent params to 0
    // Set non-writeable params to 0
    for (int i = 0; ((pmap >> i) > 0) && (i < MAX_PARAMS); i++) {
        if (dev->params[i].write == 0) {
            pmap &= ~(1 << i);  // Set bit i to 0
        }
    }
    // Build the message
    message_t* dev_write = malloc(sizeof(message_t));
    if (dev_write == NULL) {
        log_printf(FATAL, "make_device_write: Failed to malloc");
        exit(1);
    }
    dev_write->message_id = DEVICE_WRITE;
    dev_write->payload_length = 0;
    dev_write->max_payload_length = device_write_payload_size(dev_type, pmap);
    dev_write->payload = malloc(dev_write->max_payload_length);
    if (dev_write->payload == NULL) {
        log_printf(FATAL, "make_device_write: Failed to malloc");
        exit(1);
    }
    int status = 0;
    // Append the param bitmap
    status += append_payload(dev_write, (uint8_t*) &pmap, BITMAP_SIZE);
    // Append the param values to the payload
    for (int i = 0; ((pmap >> i) > 0) && (i < MAX_PARAMS); i++) {
        // If the parameter is on in the bitmap, include it
        if ((1 << i) & pmap) {
            switch (dev->params[i].type) {
                case INT:
                    status += append_payload(dev_write, (uint8_t*) &(param_values[i].p_i), sizeof(int32_t));
                    break;
                case FLOAT:
                    status += append_payload(dev_write, (uint8_t*) &(param_values[i].p_f), sizeof(float));
                    break;
                case BOOL:
                    status += append_payload(dev_write, (uint8_t*) &(param_values[i].p_b), sizeof(uint8_t));
                    break;
            }
        }
    }
    return (status == 0) ? dev_write : NULL;
}

void destroy_message(message_t* message) {
    free(message->payload);
    free(message);
}

// ********************* SERIALIZE AND PARSE MESSAGES *********************** //

size_t calc_max_cobs_msg_length(message_t* msg) {
    size_t required_packet_length = MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length + CHECKSUM_SIZE;
    // Cobs encoding a length N message adds overhead of at most ceil(N/254)
    size_t cobs_length = required_packet_length + (required_packet_length / 254) + 1;
    /* Add 2 additional bytes to the buffer for use in message_to_bytes()
     * 0th byte will be 0x0 indicating the start of a packet.
     * 1st byte will hold the actual length from cobs encoding */
    return DELIMITER_SIZE + COBS_LENGTH_SIZE + cobs_length;
}

ssize_t message_to_bytes(message_t* msg, uint8_t cobs_encoded[], size_t len) {
    size_t required_length = calc_max_cobs_msg_length(msg);
    if (len < required_length) {
        return -1;
    }
    // Build an intermediate byte array to hold the serialized message to be encoded
    uint8_t* data = malloc(MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length + CHECKSUM_SIZE);
    if (data == NULL) {
        log_printf(FATAL, "message_to_bytes: Failed to malloc");
        exit(1);
    }
    data[0] = msg->message_id;
    data[1] = msg->payload_length;
    for (int i = 0; i < msg->payload_length; i++) {
        data[i + MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE] = msg->payload[i];
    }
    data[MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length] = checksum(data, MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length);

    // Encode the intermediate byte array into output buffer
    cobs_encoded[0] = 0x00;
    int cobs_len = cobs_encode(&cobs_encoded[2], data, MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg->payload_length + CHECKSUM_SIZE);
    free(data);
    cobs_encoded[1] = cobs_len;
    return DELIMITER_SIZE + COBS_LENGTH_SIZE + cobs_len;
}

int parse_message(uint8_t data[], message_t* msg_to_fill) {
    uint8_t cobs_len = data[1];
    uint8_t* decoded = malloc(cobs_len);  // Actual number of bytes populated will be a couple less due to overhead
    if (decoded == NULL) {
        log_printf(FATAL, "parse_message: Failed to malloc");
        exit(1);
    }
    int ret = cobs_decode(decoded, &data[2], cobs_len);
    if (ret < (MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + CHECKSUM_SIZE)) {
        // Smaller than valid message
        free(decoded);
        return 3;
    } else if (ret > (MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + MAX_PAYLOAD_SIZE + CHECKSUM_SIZE)) {
        // Larger than the largest valid message
        free(decoded);
        return 3;
    }
    msg_to_fill->message_id = decoded[0];
    msg_to_fill->payload_length = 0;
    msg_to_fill->max_payload_length = decoded[MESSAGE_ID_SIZE];
    ret = append_payload(msg_to_fill, &decoded[MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE], msg_to_fill->max_payload_length);
    if (ret != 0) {
        log_printf(ERROR, "parse_message: Overwrote to payload\n");
        return 2;
    }
    uint8_t expected_checksum = checksum(decoded, MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg_to_fill->payload_length);
    uint8_t received_checksum = decoded[MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + msg_to_fill->payload_length];
    if (expected_checksum != received_checksum) {
        log_printf(ERROR, "parse_message: Expected checksum 0x%02X. Received 0x%02X\n", expected_checksum, received_checksum);
    }
    free(decoded);
    return (expected_checksum != received_checksum) ? 1 : 0;
}

void parse_device_data(uint8_t dev_type, message_t* dev_data, param_val_t vals[]) {
    device_t* dev = get_device(dev_type);
    // Bitmap is stored in the first 32 bits of the payload
    uint32_t bitmap = *((uint32_t*) dev_data->payload);
    /* Iterate through device's parameters. If bit is off, continue
     * If bit is on, determine how much to read from the payload then put it in VALS in the appropriate field */
    uint8_t* payload_ptr = &(dev_data->payload[BITMAP_SIZE]);  // Start the pointer at the beginning of the values (skip the bitmap)
    for (int i = 0; ((bitmap >> i) > 0) && (i < MAX_PARAMS); i++) {
        // If bit is on, parameter is included in the payload
        if ((1 << i) & bitmap) {
            switch (dev->params[i].type) {
                case INT:
                    vals[i].p_i = *((int32_t*) payload_ptr);
                    payload_ptr += sizeof(int32_t) / sizeof(uint8_t);
                    break;
                case FLOAT:
                    vals[i].p_f = *((float*) payload_ptr);
                    payload_ptr += sizeof(float) / sizeof(uint8_t);
                    break;
                case BOOL:
                    vals[i].p_b = *payload_ptr;
                    payload_ptr += sizeof(uint8_t) / sizeof(uint8_t);
                    break;
            }
        }
    }
}
