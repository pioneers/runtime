#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "../../runtime_util/runtime_util.h"
#include "../../net_handler/net_util.h"
#include <sys/time.h>

#define SHEPHERD_CLIENT 0
#define DAWN_CLIENT 1

#define IDLE_MODE 0
#define AUTO_MODE 1
#define TELEOP_MODE 2

#define LEFT_POS 0
#define RIGHT_POS 1

typedef struct dev_data {
	uint64_t uid;    //what the uid of this device is
	uint32_t params; //which params to subscribe to
} dev_data_t;

void start_net_handler ();

void stop_net_handler ();

void send_run_mode (int client, int mode);

void send_start_pos (int client, int pos);

void send_challenge_data (int client, char **data);

void send_device_data (dev_data_t *data);

#endif