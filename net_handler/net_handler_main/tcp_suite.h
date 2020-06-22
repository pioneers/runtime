#ifndef TCP_SUITE_H
#define TCP_SUITE_H

#include "../runtime_util/runtime_util.h"

//start the threads managing a TCP connection
void start_tcp_suite (target_t target);

//stop the threads managing the TCP connection
void stop_tcp_suite (target_t target);

#endif