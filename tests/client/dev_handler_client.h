#ifndef DEV_CLIENT_H
#define DEV_CLIENT_H

#include "../../runtime_util/runtime_util.h"
#include <stdlib.h>
#include <signal.h>

// Starts dev handler with "virtual" argument
void start_dev_handler();

// Stops dev handler
void stop_dev_handler();

/**
 * Connects a virtual device to dev handler
 * device_name: The name of a virtual device
 * Returns the port number at which the device is connected
 */
void connect_device(char* device_name);

/**
 * Disconnects a virtual device from dev handler
 * port_number: The port number returned from connect_device()
 */
void disconnect_device(int port_number);

/**
 * Prints the list of connected ports and the corresponding virtual device
 */
void list_devices();

#endif
