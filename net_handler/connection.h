#ifndef TCP_CONN_H
#define TCP_CONN_H

#include <net_handler_message.h>
#include <net_util.h>

/**
 * Start the main TCP connection control thread. Does not block.
 * Should be called when a TCP client has requested a connection to Runtime.
 *
 * Args:
 *  - client: client to communicate with, either DAWN or SHEPHERD
 *  - connfd: connection socket descriptor on which there is the established connection with Dawn
 *  - send_logs: whether to send logs over TCP to client. DAWN should receive logs and SHEPHERD should not
 */
void start_tcp_conn(robot_desc_field_t client, int connfd, int send_logs);

/**
 * Stops the TCP connection control thread cleanly. May block briefly to allow
 * main control thread to finish what it's doing. Should be called right before the net_handler main loop terminates.
 *
 * Args:
 *  - client: which client to stop communicating with, either DAWN or SHEPHERD
 */
void stop_tcp_conn(robot_desc_field_t client);

#endif
