/**
 *  A simple dev handler to read and write over serial using fread/fwrite
 */
#include "fake_handler.h"

// The string name of the serial port
#define DEVICE "/dev/ttyACM0"
// The baud rate set on the Arduino
#define BAUD_RATE 115200
// How long we want to wait for Arduino to respond to us
#define TIMEOUT 2000
// Interval that we send HeartBeat requests to Arduino
#define HB_INTERVAL 3000

int send_ping(int fd) {
    message_t* ping = make_ping();
    int len = calc_max_cobs_msg_length(ping);
    uint8_t* data = malloc(len);
    len = message_to_bytes(ping, data, len);
    int ret;
    // Write each byte of data one at a time
    for (int i = 0; i < len; i++) {
        ret = serialport_write(fd, data[i]); // From arduino-serial-lib
        if (ret != 0) {
            printf("Couldn't write byte 0x%X\n", data[i]);
            return -1;
        }
        printf("Wrote byte 0x%X\n", data[i]);
    }
    free(data);
    return 0;
}

int wait_sub_response(int fd) {
    int byte_read;
    uint8_t* data = malloc(100); // Aribitrarily big
    int len = 0;
    int num_bytes_read
    // Read one byte at a time until we get 0x00
    while (1) {
        num_bytes_read = read(fd, &byte_read, 1);  // read a char at a time
        if (num_bytes_read == 1) {
            printf("Got something\n");
            if (byte_read == 0x00) {
                printf("Got the start of a message\n");
                break;
            }
        }
    }
    // Start filling data array
    data[0] = last_byte_read;
    num_bytes_read = read(fd, &data[1], 1); // Read the cobs length
    if (num_bytes_read != 1) {
        printf("Couldn't Read the cobs len\n");
        return 1;
    }
    printf("Got cobs_len: %d\n", data[1]);
    // Read the rest of the message
    num_bytes_read = read(fd, &data[2], cobs_len);
    if (num_bytes_read != cobs_len) {
        printf("Couldn't read the entire message\n");
        return 1;
    }
    // Now we have the entire byte array
    printf("Received message: 0x");
    for (int i = 0; i < 2 + cobs_len; i++) {
        printf("%X", data[i]);
    }
    printf("\n");
    return 0;
}

int main() {
    // Open the serial port
    int fd = serialport_init(DEVICE, BAUD_RATE);
    if (fd == -1) {
        printf("Couldn't open serial port\n");
        return 1;
    }
    printf("FD: %d\n", fd);
    int ret;
    // Send a ping
    ret = send_ping(fd);
    if (ret != 0) {
        printf("Couldn't send ping!\n");
        return 2;
    }
    // Get the subscription response
    ret = wait_sub_response(fd);
    if (ret != 0) {
        printf("Didn't get SubscriptionResponse!\n");
        return 3;
    }
    printf("Got SubscriptionResponse!\n");
    serialport_close(fd);
    return 0;
}
