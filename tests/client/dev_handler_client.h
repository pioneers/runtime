#ifndef DEV_CLIENT_H
#define DEV_CLIENT_H

#include <stdlib.h>     // for exit()
#include <stdio.h>      // for printf()
#include <errno.h>      // for errno
#include <string.h>     // for strerr()
#include <sys/socket.h> // for sockets
#include <sys/un.h>     // for sockaddr_un
#include <stdint.h>     // for int with specific widths
#include <unistd.h>     // read()
#include <signal.h>     // for SIGINT (Ctrl+C)

// Starts dev handler with "virtual" argument
void start_dev_handler();

// Stops dev handler
void stop_dev_handler();

/**
 * Connects a virtual device to dev handler
 * Arguments:
 *    device_name: The name of a virtual device
 */
void connect_device(char* device_name);

/**
 * Disconnects a virtual device from dev handler
 * Arguments:
 *    port_number: The port number returned from connect_device()
 */
void disconnect_device(int port_number);

/**
 * Prints the list of connected ports and the corresponding virtual device
 */
void list_devices();

#endif
