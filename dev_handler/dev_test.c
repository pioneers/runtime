/**
 * Test file to make sure that port numbers remain stable for the lifetime of a connected device
 */

#include <libusb.h>
// #include "../runtime_util/runtime_util.h"
// #include "string.h" // strcmp
#include <stdio.h> // Print
#include <stdlib.h>
#include <stdint.h> // uint8_t types
#include <unistd.h>  // Sleep
// #include <signal.h> // Used to handle SIGTERM, SIGINT, SIGKILL
// #include <time.h>   // For timestamps on device connects/disconnects

void print_used_ports(libusb_device** lst, int len) {
    printf("INFO: Used ports:\n");
    int ret;
    struct libusb_device_descriptor *desc = (struct libusb_device_descriptor *) malloc (sizeof(struct libusb_device_descriptor) * 1);
    for (int i = 0; i < len; i++) {
        libusb_device* dev = lst[i];
        // Get device descriptor
        if ((ret = libusb_get_device_descriptor(dev, desc)) != 0) {
            printf("libusb_get_device_descriptor failed on exit code %d\n", ret);
        }
        // Print device info
        printf("Device #%d\n", i);
        printf("    vendor ID = %d\n", desc->idVendor);
        printf("    Product ID = %d\n", desc->idProduct);
        // Print port number
        printf("    Port = %d\n\n", libusb_get_port_number(dev));
    }
    free(desc);
}

// Notify when a new device is connected/disconnected and print the used ports
void poll() {
    libusb_device** lst;
    int len;
    int i = 0;
    while (1) {
        printf("======ITERATION %d======\n", i);
        len = libusb_get_device_list(NULL, &lst);
        print_used_ports(lst, len);
        sleep(3);
        i++;
    }
}

int main() {
    printf("===Starting dev_test===\n");
    libusb_init(NULL);
    poll();
}
