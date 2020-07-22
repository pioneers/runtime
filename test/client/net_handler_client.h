#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "../../runtime_util/runtime_util.h"
#include "../../net_handler/net_util.h"

#define SHEPHERD_CLIENT 0
#define DAWN_CLIENT 1

void usage ();

void start_net_handler ();

void stop_net_handler ();

#endif