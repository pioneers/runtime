#ifndef UDP_SUITE_H
#define UDP_SUITE_H

#include <sys/socket.h>         // for sockets
#include <arpa/inet.h>          // for ip addresses
#include <pthread.h>            // for pthreads
#include <signal.h>             // for signal

#include "../runtime_util/runtime_util.h"
#include "../shm_wrapper/shm_wrapper.h"
#include "../shm_wrapper_aux/shm_wrapper_aux.h"
#include "net_util.h"

#include "pbc_gen/gamepad.pb-c.h"
#include "pbc_gen/device.pb-c.h"

//start the threads managing a UDP connection
void start_udp_conn ();

//stop the threads managing the UDP connection
void stop_udp_conn ();

#endif