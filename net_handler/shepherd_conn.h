#ifndef SHEP_CONN_H
#define SHEP_CONN_H

#include "net_util.h"

//starts the shepherd connection thread
//connfd is the file descriptor of the connected socket returned by accept()
void start_shepherd_conn (int connfd);

//gracefully stops the shepherd connection thread
void stop_shepherd_conn ();

#endif