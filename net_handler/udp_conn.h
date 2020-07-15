#ifndef UDP_SUITE_H
#define UDP_SUITE_H

#include "net_util.h"

//start the threads managing the UDP connection
void start_udp_conn();

//stop the threads managing the UDP connection
void stop_udp_conn();

#endif