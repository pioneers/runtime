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
 *    dev_exe_name: The name of a virtual device's exe file in /virtual_devices dir
 * Returns:
 *    0 on success
 *    -1 on failure
 */
int connect_device(char* dev_exe_name);

/**
 * Disconnects a virtual device from dev handler
 * Arguments:
 *    socket_num: The port number returned from connect_device()
 * Returns:
 *    0 on success
 *    -1 if invalid socket number
 */
int disconnect_device(int socket_num);

/**
 * Disconnects all currently connected virtual devices
 */
void disconnect_all_devices();

/**
 * Prints the list of connected ports and the corresponding virtual device
 */
void list_devices();

#endif
