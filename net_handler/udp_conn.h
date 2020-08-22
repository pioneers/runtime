#ifndef UDP_SUITE_H
#define UDP_SUITE_H

#include "net_util.h"

/**
 * Start the gamepad and device threads managing the UDP connection
 */
void start_udp_conn();

/**
 * Stop the gamepad and device threads managing the UDP connection
 */
void stop_udp_conn();

#endif