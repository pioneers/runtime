#ifndef LOG_RELAY_H
#define LOG_RELAY_H

#include "net_util.h"

typedef struct log_args {
	int fifo_fd;
	pthread_cond_t *new_logs_cond;
	pthread_mutex_t *new_logs_mutex;
	int newest_log_num;
	char **logs;
} log_relay_args_t;

void start_log_relayer (log_relay_args_t *args);

#endif