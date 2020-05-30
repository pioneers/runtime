#include "message.h"

// Runs a test to make sure that a LOG message works
void test_log(char* payload) {
    // Device wants to send "Hello" to DEV_HANDLER
    printf("Converting the message [%s] to bytes\n", payload);
    message_t* device_message = make_log(payload);
    // Deconstruct it into bytes
    int len = 3 + strlen(payload) + 1;
    char bytes[len];
    message_to_bytes(device_message, bytes, len); // bytes[] is "Hello this is from the device" in 1's and 0's
    // Print out the Message containing "Hello" in hex
    printf("Message Type: %X\n", bytes[0]);
    printf("Payload Length: %X\n", bytes[1]);
    printf("Payload: ");
    for (int i = 2; i < len - 2; i++) {
        printf("%X", bytes[i]);
    }
    printf("\n");
    printf("Checksum: %X\n", bytes[len - 1]);

    /* DEVICE SENDS bytes[] TO DEV_HANDLER */
    /* USB STUFF HAPPENS HERE */
    printf("\n");
    printf("Attempting to parse the bytes back into a message\n");
    // Reconstruct bytes into a message
    message_t* received_message = malloc(sizeof(message_t));
    received_message->payload = malloc(bytes[1]);
    int status = parse_message(bytes, received_message);
    printf("Finished parse attempt\n");
    if (status == 0) {
        printf("Successful message parse.\n");
        printf("Parsed message: %s\n", received_message->payload);
        if (strcmp(payload, received_message->payload) == 0) {
            printf("SUCCESSFUL LOG\n");
        }
    } else {
        printf("Unsuccessful message parse\n");
    }
    destroy_message(device_message);
    free(received_message->payload);
    free(received_message);
}

// Runs a test on DeviceData message
void test_device_data() {
    param_val_t* param_values[3];
    param_values[0] = malloc(sizeof(param_val_t));
    param_values[0]->p_b = 0;
    param_values[1] = malloc(sizeof(param_val_t));
    param_values[1]->p_b = 1;
    param_values[2] = malloc(sizeof(param_val_t));
    param_values[2]->p_b = 1;
    printf("**************\n");
    dev_id_t* device_id = malloc(sizeof(dev_id_t));
    device_id->type = 0;
    device_id->year = 20;
    device_id->uid = 0xBEEF;
    printf("Forming DeviceData message\n");
    message_t* dev_data = make_device_data(device_id, param_values, 3);
    printf("dev_data message_id (should be 0x15): %X\n", dev_data->message_id);
    printf("dev_data payload_length: %X\n", dev_data->payload_length);
    printf("dev_data payload: %X\n", dev_data->payload);
    // Turn the message into bytes
    printf("Converting device data message into bytes\n");
    int len = 1 + 1 + dev_data->payload_length + 1;
    char bytes[len];
    message_to_bytes(dev_data, bytes, len);

    printf("Attempting to parse the bytes back into a message\n");

    message_t* received_message = malloc(sizeof(message_t));
    received_message->payload = calloc(132, sizeof(char));
    int status = parse_message(bytes, received_message);
    if (status == 0) {
        printf("Successful parse\n");
    } else {
        printf("Unsuccessful parse\n");
    }
    printf("Param bit mask: %X %X %X %X\n", received_message->payload[3], received_message->payload[2], received_message->payload[1], received_message->payload[0]);
    printf("switch0 value: %X\n", received_message->payload[4]);
    printf("switch1 value: %X\n", received_message->payload[5]);
    printf("switch2 value: %X\n", received_message->payload[6]);

    for (int i = 0; i < 3; i++) {
        free(param_values[i]);
    }
    free(device_id);
    destroy_message(dev_data);
    free(received_message->payload);
    free(received_message);
}

int main() {
    printf("===TEST LOG===\n");
    test_log("Hello");
    printf("\n");
    printf("===TEST DEVICE DATA===\n");
    test_device_data();
}
