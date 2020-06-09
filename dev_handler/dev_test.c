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
    int ret;
    struct libusb_device_descriptor *desc = (struct libusb_device_descriptor *) malloc (sizeof(struct libusb_device_descriptor) * 1);
    printf("Device#   Port  Vendor:Product\n");
    for (int i = 0; i < len; i++) {
        libusb_device* dev = lst[i];
        // Get device descriptor
        if ((ret = libusb_get_device_descriptor(dev, desc)) != 0) {
            printf("libusb_get_device_descriptor failed on exit code %d\n", ret);
        }
        // Print device info
        printf("   %d       %d       %d:%d\n", i, libusb_get_port_number(dev), desc->idVendor, desc->idProduct);
    }
    free(desc);
}

// Notify when a new device is connected/disconnected and print the used ports
void poll() {
    libusb_device** lst;
    int len;
    int i = 0;
    while (1) {
        printf("\n=========ITERATION %d=========\n", i);
        len = libusb_get_device_list(NULL, &lst);
        print_used_ports(lst, len);
        i++;
        sleep(3);
    }
}

int main() {
    printf("===Starting dev_test===\n");
    libusb_init(NULL);
    poll();
}
