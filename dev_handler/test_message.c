#include "message.h"

/* make_log test */
void test_log(char* payload) {
    // Device wants to send payload to DEV_HANDLER
    printf("Converting the message [%s] to bytes\n", payload);
    message_t* device_message = make_log(payload);
    // Deconstruct it into bytes
    int len = 3 + strlen(payload) + 1;
    uint8_t bytes[len];
    message_to_bytes(device_message, bytes, len);
    // Print out the bytes
    printf("Message Type: %X\n", bytes[0]);
    printf("Payload Length: %X\n", bytes[1]);
    printf("Payload: ");
    for (int i = 2; i < len - 2; i++) {
        printf("%X", bytes[i]);
    }
    printf("\n");
    printf("Checksum: %X\n", bytes[len - 1]);

    // Bytes --> Message
    printf("Attempting to parse the bytes back into a message\n");
    // Reconstruct bytes into a message
    message_t* received_message = make_empty(bytes[1]);
    parse_message(bytes, received_message);
    printf("Parsed message: %s\n", received_message->payload);
    if (strcmp(payload, (char*) received_message->payload) == 0) {
        printf("***SUCCESS***\n");
    }

    destroy_message(device_message);
    destroy_message(received_message);
}

// Runs a test on DeviceData message
void test_device_data() {
    // LimitSwitch
    device_t* device = get_device(13);
    param_val_t* param_values[device->num_params];
    make_empty_param_values(device->type, param_values);
    param_values[0]->p_f = 0.2345;
    param_values[1]->p_f = 13245.2;
    param_values[2]->p_f = 76523.456;
    param_values[3]->p_f = 321.123;
    param_values[4]->p_f = 1337.000;
    param_values[5]->p_f = 123.45;
    param_values[6]->p_b = 1;
    param_values[7]->p_i = 777;
    param_values[8]->p_f = 7654.2435;
    param_values[9]->p_f = 875.4356;
    param_values[10]->p_f = 52345.4532;
    param_values[11]->p_f = 1212.34;
    param_values[12]->p_f = 82094.34;
    param_values[13]->p_f = 69919.22;
    param_values[14]->p_b = 1;
    param_values[15]->p_i = 33;
    dev_id_t device_id = {
        .type = 13,
        .year = 20,
        .uid = 0xBEEF
    };

    // Build the message from test case inputs
    printf("***Forming DeviceData message***\n");
    message_t* dev_data = make_device_data(&device_id, param_values, device->num_params);
    printf("dev_data message_id (should be 0x15): %X\n", dev_data->message_id);
    printf("dev_data payload_length: %X\n", dev_data->payload_length);
    printf("dev_data payload: ");
    for (int i = 0; i < dev_data->payload_length; i++) {
        printf("%X", dev_data->payload[i]);
    }
    printf("\n");

    // Turn the message into bytes
    printf("********\n");
    printf("Converting device data message into bytes: ");
    int len = 1 + 1 + dev_data->payload_length + 1;
    uint8_t bytes[len];
    message_to_bytes(dev_data, bytes, len);
    for (int i = 0; i < len; i++) {
        printf("%X", bytes[i]);
    }
    printf("\n");

    // Turn bytes into message
    printf("********\n");
    printf("Attempting to parse the bytes back into a message\n");
    message_t* received_message = make_empty(132);
    parse_message(bytes, received_message);

    printf("Received payload: ");
    for (int i = 0; i < received_message->payload_length; i++) {
        printf("%X", received_message->payload[i]);
    }
    printf("\n");

    // Read out the encoded values
    printf("********\n");
    param_val_t* vals[device->num_params];
    make_empty_param_values(device->type, vals);
    parse_device_data(device->type, received_message, vals);
    int bit_mask = ((int*) received_message->payload)[0];
    printf("Param bit mask: %X\n", bit_mask);
    for (int i = 0; i < device->num_params; i++) {
        // If i-th bit from the right is off, don't print out the value
        // if (((1 << i) & bit_mask) == 0) {
        //     printf("%s is not to be read from\n", device->params[i].name);
        // }
        if (strcmp(device->params[i].type, "int") == 0) {
            printf("%s: %d\n", device->params[i].name, vals[i]->p_i);
        } else if (strcmp(device->params[i].type, "float") == 0) {
            printf("%s: %f\n", device->params[i].name, vals[i]->p_f);
        } else if (strcmp(device->params[i].type, "bool") == 0) {
            printf("%s: %x\n", device->params[i].name, vals[i]->p_b);
        }
    }

    // Free stuff
    destroy_param_values(device->type, param_values);
    destroy_message(dev_data);
    destroy_message(received_message);
    destroy_param_values(device->type, vals);
}

int main() {
    printf("===TEST LOG===\n");
    test_log("Hello");
    printf("\n");
    printf("===TEST DEVICE DATA===\n");
    test_device_data();
}
