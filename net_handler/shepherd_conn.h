#ifndef SHEP_CONN_H
#define SHEP_CONN_H

#include "net_util.h"

/*
 * Start the main shepherd connection control thread. Does not block.
 * Should be called when Shepherd has requested a connection to Runtime.
 * Arguments:
 *    - int connfd: connection socket descriptor on which there is the established connection with Shepherd
 */
void start_shepherd_conn (int connfd);

/*
 * Stops the shepherd connection control thread cleanly. May block briefly to allow
 * main control thread to finish what it's doing. Should be called right before the net_handler main loop terminates.
 */
void stop_shepherd_conn ();

#endif