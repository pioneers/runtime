#ifndef TCP_CONN_H
#define TCP_CONN_H

#include "net_util.h"

/*
 * Start the main TCP connection control thread. Does not block.
 * Should be called when a TCP client has requested a connection to Runtime.
 * Arguments:
 *    - int connfd: connection socket descriptor on which there is the established connection with Dawn
 */
void start_tcp_conn (robot_desc_field_t client, int connfd, int send_logs);

/*
 * Stops the TCP connection control thread cleanly. May block briefly to allow
 * main control thread to finish what it's doing. Should be called right before the net_handler main loop terminates.
 */
void stop_tcp_conn (robot_desc_field_t client);

#endif