#include "general_dev.h"

// How often we send PING to dev handler
#define PING_FREQ 1000

// Longest time period we tolerate not having a PING
#define TIMEOUT 2500

// Boolean value indicating whether we sent an ACK
uint8_t sent_ack = 0;

// ************************ GENERAL DEVICE SPECIFIC ************************* //

// GeneralTestDevice params
enum {
    // Read-only
    INCREASING_ODD, DECREASING_ODD, INCREASING_EVEN, DECREASING_EVEN,
    INCREASING_FLIP, ALWAYS_LEET, DOUBLING, DOUBLING_NEG, HALFING,
    HALFING_NEG, EXP_ONE_PT_ONE, EXP_ONE_PT_TWO, ALWAYS_PI, FLIP_FLOP,
    ALWAYS_TRUE, ALWAYS_FALSE,
    // Read and Write
    RED_INT, ORANGE_INT, GREEN_INT, BLUE_INT, PURPLE_INT,
    RED_FLOAT, ORANGE_FLOAT, GREEN_FLOAT, BLUE_FLOAT, PURPLE_FLOAT,
    RED_BOOL, ORANGE_BOOL, GREEN_BOOL, BLUE_BOOL, PURPLE_BOOL,
    YELLOW_BOOL
};

/**
 * Initialize the values for each param
 * Arguments:
 *    params: Array of params to be initialized
 */
static void init_params(param_val_t params[]) {
    params[INCREASING_ODD].p_i      = 1;
    params[DECREASING_ODD].p_i      = -1;
    params[INCREASING_EVEN].p_i     = 0;
    params[DECREASING_EVEN].p_i     = 0;
    params[INCREASING_FLIP].p_i     = 0;
    params[ALWAYS_LEET].p_i         = 1337;
    params[DOUBLING].p_f            = 1;
    params[DOUBLING_NEG].p_f        = -1;
    params[HALFING].p_f             = 1048576; // 2^20
    params[HALFING_NEG].p_f         = -1048576;
    params[EXP_ONE_PT_ONE].p_f      = 1;
    params[EXP_ONE_PT_TWO].p_f      = 1;
    params[ALWAYS_PI].p_f           = 3.14159265359;
    params[FLIP_FLOP].p_b           = 1;
    params[ALWAYS_TRUE].p_b         = 1;
    params[ALWAYS_FALSE].p_b        = 0;
    // Read and Write
    params[RED_INT].p_i         = 1;
    params[ORANGE_INT].p_i      = 2;
    params[GREEN_INT].p_i       = 3;
    params[BLUE_INT].p_i        = 4;
    params[PURPLE_INT].p_i      = 5;
    params[RED_FLOAT].p_f       = 6.6;
    params[ORANGE_FLOAT].p_f    = 7.7;
    params[GREEN_FLOAT].p_f     = 8.8;
    params[BLUE_FLOAT].p_f      = 9.9;
    params[PURPLE_FLOAT].p_f    = 10.1;
    params[RED_BOOL].p_b        = 1;
    params[ORANGE_BOOL].p_b     = 1;
    params[GREEN_BOOL].p_b      = 1;
    params[BLUE_BOOL].p_b       = 1;
    params[PURPLE_BOOL].p_b     = 1;
    params[YELLOW_BOOL].p_b     = 1;
}

/**
 * Changes device's read-only params
 * Arguments:
 *    params: Array of param values to be modified
 */
static void device_actions(param_val_t params[]) {
    params[INCREASING_ODD].p_i += 2;
    params[DECREASING_ODD].p_i -= 2;
    params[INCREASING_EVEN].p_i += 2;
    params[DECREASING_EVEN].p_i -= 2;
    params[INCREASING_FLIP].p_i = (-1) *
        (params[INCREASING_FLIP].p_i + ((params[INCREASING_FLIP].p_i % 2 == 0) ? 1 : -1)); // -1, 2, -3, 4, etc

    params[DOUBLING].p_f *= 2;
    params[DOUBLING_NEG].p_f *= 2;
    params[HALFING].p_f /= 2; // 2^20
    params[HALFING_NEG].p_f /= 2;
    params[EXP_ONE_PT_ONE].p_f *= 1.1;
    params[EXP_ONE_PT_TWO].p_f *= 1.2;
    params[FLIP_FLOP].p_b = 1 - params[FLIP_FLOP].p_b;
}

// **************************** PUBLIC FUNCTIONS **************************** //

message_t *make_acknowledgement(uint8_t type, uint8_t year, uint64_t uid) {
    message_t *msg = malloc(sizeof(message_t));
    msg->message_id = ACKNOWLEDGEMENT;
    msg->max_payload_length = DEVICE_ID_SIZE;
    msg->payload = malloc(msg->max_payload_length);
    msg->payload_length = DEVICE_ID_SIZE;
    msg->payload[0] = type;
    msg->payload[1] = year;
    memcpy(&msg->payload[2], &uid, 8);
    return msg;
}

int receive_message(int fd, message_t *msg) {
    uint8_t last_byte_read = 0; // Variable to temporarily hold a read byte
    int num_bytes_read = 0;

    // Keep reading a byte until we get the delimiter byte
    while (1) {
        num_bytes_read = read(fd, &last_byte_read, 1);   // Waiting for first byte can block
        if (num_bytes_read == -1) {
            printf("Couldn't read first byte -- %s\n", strerror(errno));
            return 1;
        } else if (last_byte_read == 0x00) {
            // Found start of a message
            break;
        }
        // If we were able to read a byte but it wasn't the delimiter
        // printf("Attempting to read delimiter but got 0x%02X\n", last_byte_read);
    }

    // Read the next byte, which tells how many bytes left are in the message
    uint8_t cobs_len;
    num_bytes_read = read(fd, &cobs_len, 1);
    if (num_bytes_read != 1) {
        return 1;
    }

    // Allocate buffer to read message into
    uint8_t *data = malloc(DELIMITER_SIZE + COBS_LENGTH_SIZE + cobs_len);
    data[0] = 0x00;
    data[1] = cobs_len;

    // Read the message
    num_bytes_read = read(fd, &data[2], cobs_len);
    if (num_bytes_read != cobs_len) {
        printf("Couldn't read the full message. Read only %d out of %d bytes\n", num_bytes_read, cobs_len);
        free(data);
        return 1;
    }

    // Parse the message
    int ret = parse_message(data, msg);
    free(data);
    if (ret != 0) {
        printf("Incorrect checksum\n");
        return 2;
    }
    return 0;
}

void send_message(int fd, message_t *msg) {
    int len = calc_max_cobs_msg_length(msg);
    uint8_t *data = malloc(len);
    len = message_to_bytes(msg, data, len);
    int transferred = write(fd, data, len);
    if (transferred != len) {
        printf("Sent only %d out of %d bytes\n", transferred, len);
    }
    free(data);
}

void device_write(uint8_t type, message_t *dev_write, param_val_t params[]) {
    device_t *dev = get_device(type);
    // Get bitmap from payload
    uint32_t pmap;
    memcpy(&pmap, &dev_write->payload[0], BITMAP_SIZE);
    // Process each parameter
    uint8_t *payload_ptr = &dev_write->payload[BITMAP_SIZE];
    for (int i = 0; ((pmap >> i) > 0) && (i < 32); i++) {
        if (pmap & (1 << i)) {
            // Write to the corresponding field in params[i]
            switch (dev->params[i].type) {
                case INT:
                    // TODO: Check int size
                    params[i].p_i = *((uint16_t *) payload_ptr);
                    payload_ptr += sizeof(uint16_t);
                    break;
                case FLOAT:
                    params[i].p_f = *((float *) payload_ptr);
                    payload_ptr += sizeof(float);
                    break;
                case BOOL:
                    params[i].p_b = *((uint8_t *) payload_ptr);
                    payload_ptr += sizeof(uint8_t);
                    break;
            }
        }
    }
}

message_t *make_device_data(uint8_t type, uint32_t pmap, param_val_t params[]) {
    message_t *dev_data = make_empty(MAX_PAYLOAD_SIZE);
    dev_data->message_id = DEVICE_DATA;
    // Copy pmap into payload
    memcpy(&dev_data->payload[0], &pmap, BITMAP_SIZE);
    dev_data->payload_length = BITMAP_SIZE;
    // Copy params into payload
    device_t *dev = get_device(type);
    uint8_t *payload_ptr = &dev_data->payload[BITMAP_SIZE];
    for (int i = 0; ((pmap >> i) > 0) && (i < MAX_PARAMS); i++) {
        if (pmap & (1 << i)) {
            switch(dev->params[i].type) {
                case INT:
                    memcpy(payload_ptr, &params[i].p_i, sizeof(uint16_t));
                    payload_ptr += sizeof(uint16_t);
                    dev_data->payload_length += sizeof(uint16_t);
                    break;
                case FLOAT:
                    memcpy(payload_ptr, &params[i].p_f, sizeof(float));
                    payload_ptr += sizeof(float);
                    dev_data->payload_length += sizeof(float);
                    break;
                case BOOL:
                    memcpy(payload_ptr, &params[i].p_b, sizeof(uint8_t));
                    payload_ptr += sizeof(uint8_t);
                    dev_data->payload_length += sizeof(uint8_t);
                    break;
            }
        }
    }
    // This field is useless for this function but we'll set it to be consistent
    dev_data->max_payload_length = dev_data->payload_length;
    return dev_data;
}

// ********************************* MAIN *********************************** //

void lowcar_protocol(int fd, uint8_t type, uint8_t year, uint64_t uid, param_val_t params[]) {
    // Every cycle, read a message and respond accordingly, then send messages as needed
    message_t *incoming_msg = make_empty(MAX_PAYLOAD_SIZE);
    message_t *outgoing_msg;
    uint64_t last_sent_ping_time = 0;
    uint64_t last_received_ping_time = millis();
    uint64_t last_sent_data_time = 0;
    uint32_t subscribed_params = 0;
    uint16_t subscription_interval = 0;
    uint64_t last_device_action = 0;
    uint64_t now;
    while (1) {
        now = millis();
        if (receive_message(fd, incoming_msg) == 0) {
            // Got a message
            switch (incoming_msg->message_id) {
                case PING:
                    // printf("Got PING\n");
                    last_received_ping_time = now;
                    if (!sent_ack) {
                        // Send an ack
                        outgoing_msg = make_acknowledgement(type, year, uid);
                        send_message(fd, outgoing_msg);
                        destroy_message(outgoing_msg);
                        sent_ack = 1;
                    }
                    break;

                case SUBSCRIPTION_REQUEST:
                    memcpy(&subscribed_params, &incoming_msg->payload[0], BITMAP_SIZE);
                    memcpy(&subscription_interval, &incoming_msg->payload[BITMAP_SIZE], INTERVAL_SIZE);
                    // printf("Now subscribed to 0x%08X every %d milliseconds\n", subscribed_params, subscription_interval);
                    break;

                case DEVICE_WRITE:
                    // printf("Got DEVICE_WRITE for 0x%08X\n", *((uint32_t *) incoming_msg->payload));
                    device_write(type, incoming_msg, params);
                    break;
            }
        }
        incoming_msg->message_id = NOP;
        incoming_msg->payload_length = 0;
        incoming_msg->max_payload_length = MAX_PAYLOAD_SIZE;
        memset(incoming_msg->payload, 0, MAX_PAYLOAD_SIZE);

        // Make sure we're receiving PING messages still
        if ((now - last_received_ping_time) > TIMEOUT) {
            printf("DEV_HANDLER timed out!\n");
            exit(1);
        }
        // Check if we should send another PING
        if ((now - last_sent_ping_time) > PING_FREQ) {
            // printf("Sending PING\n");
            outgoing_msg = make_ping();
            send_message(fd, outgoing_msg);
            destroy_message(outgoing_msg);
            last_sent_ping_time = now;
        }
        // Check if we should send another DEVICE_DATA
        if (subscribed_params && ((now - last_sent_data_time) > subscription_interval)) {
            // printf("Sending DEVICE_DATA with bitmap 0x%08X\n", subscribed_params);
            outgoing_msg = make_device_data(type, subscribed_params, params);
            send_message(fd, outgoing_msg);
            destroy_message(outgoing_msg);
        }
        // Change read-only params every 2 seconds
        if ((now - last_device_action) > 2000) {
            device_actions(params);
            last_device_action = now;
        }
    }
}

/**
 * A device that behaves like a lowcar device, connected to dev handler via a socket
 * Arguments:
 *    int: file descriptor for the socket
 *    uint64_t: device uid
 */
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Incorrect number of arguments: %d out of %d\n", argc, 3);
        exit(1);
    }

    int fd = atoi(argv[1]);
    uint64_t uid = strtoull(argv[2], NULL, 0);

    uint8_t dev_type = 0xFF;
    device_t *dev = get_device(dev_type);

    param_val_t params[dev->num_params];
    init_params(params);

    lowcar_protocol(fd, dev_type, dev_type, uid, params);
    return 0;
}
