#ifndef UDP_SUITE_H
#define UDP_SUITE_H

#include "../runtime_util/runtime_util.h"

//start the threads managing a UDP connection
void start_udp_suite ();

//stop the threads managing the UDP connection
void stop_udp_suite ();

#endif