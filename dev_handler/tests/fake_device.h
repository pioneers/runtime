/**
 *  A fake device that reads incoming data from to_device.txt and sends data to to_dev_handler.txt
 *  Those two txt files will act as data streams containing encoded data using the protocol as the dev handler
 *  would use when communcating with a normal USB device
 */

#include <pthread.h>
#include "../../runtime_util/runtime_util.h"
#include "../message.h"
#include <stdio.h> // Print
#include <stdlib.h>
#include <stdint.h> // ints with specified sizes (uint8_t, uint16_t, etc.)
#include <time.h>   // For timestamps on HeartBeatRequests
#include <unistd.h> // sleep(int seconds)

// The file to read from
#define TO_DEVICE "to_device.txt"

// The file to write to
#define TO_DEV_HANDLER "to_dev_handler.txt"

// The fake device's ID numbers
#define TYPE 0x0000             // 16-bit
#define YEAR 0x01               // 8-bit
#define UID 0x0123456789ABCDEF  // 64-bit

// The maximum time (in milliseconds) that we'll wait for the DEV_HANDLER to respond to our HeartBeatRequest
#define TIMEOUT 1000

// Interval (in milliseconds) at which we send a HeartBeatRequest
#define HB_REQ_FREQ 2000

// "Power on" the device
void init();
