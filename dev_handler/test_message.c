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
    // Koala Bear
    device_t* device = get_device(13);
    uint32_t param_bitmap = 0x1C3; // Params 0 (float), 1 (float), 6 (bool), 7 (int), 8 (float) == 0b  0001 1100 0011 == 0x1C3
    param_val_t** vals = make_empty_param_values(param_bitmap);
    vals[0]->p_f = 0.2345;      // Param 0: duty_cycle_a
    vals[1]->p_f = 13245.2;     // Param 1: duty_cycle_b
    vals[2]->p_b = 1;           // Param 6: motor_enabled_a
    vals[3]->p_i = 777;         // Param 7: drive_mode_a
    vals[4]->p_f = 7654.2435;   // Param 8: pid_kp_a
    dev_id_t device_id = {
        .type = 13,
        .year = 20,
        .uid = 0xBEEF
    };

    // Build the message from test case inputs
    printf("***Forming DeviceData message***\n");
    message_t* dev_data = make_device_data(&device_id, param_bitmap, vals);
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
    uint32_t received_param_bitmap = ((uint32_t*) received_message->payload)[0];
    printf("Param bit mask: %X\n", received_param_bitmap);
    param_val_t** received_vals = make_empty_param_values(received_param_bitmap); // Gets the 0-th index of the payload, which is the pmap
    parse_device_data(device->type, received_message, received_vals);
    int vals_idx = 0;
    for (int i = 0; i < device->num_params; i++) {
        if (((1 << i) & received_param_bitmap) == 0) {
            continue;
        }
        if (strcmp(device->params[i].type, "int") == 0) {
            printf("%s: %d\n", device->params[i].name, vals[vals_idx]->p_i);
        } else if (strcmp(device->params[i].type, "float") == 0) {
            printf("%s: %f\n", device->params[i].name, vals[vals_idx]->p_f);
        } else if (strcmp(device->params[i].type, "bool") == 0) {
            printf("%s: %x\n", device->params[i].name, vals[vals_idx]->p_b);
        }
        vals_idx++;
    }

    // Free stuff
    destroy_param_values(device->type, vals);
    destroy_message(dev_data);
    destroy_message(received_message);
    destroy_param_values(device->type, received_vals);
}

int main() {
    printf("===TEST LOG===\n");
    test_log("Hello");
    printf("\n");
    printf("===TEST DEVICE DATA===\n");
    test_device_data();
}
