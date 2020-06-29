#ifndef DAWN_CONN_H
#define DAWN_CONN_H

#include "net_util.h"

/*
 * Start the main dawn connection control thread. Does not block.
 * Should be called when Dawn has requested a connection to Runtime.
 * Arguments:
 *    - int connfd: connection socket descriptor on which there is the established connection with Dawn
 */
void start_dawn_conn (int connfd);

/*
 * Stops the dawn connection control thread cleanly. May block briefly to allow
 * main control thread to finish what it's doing. Should be called right before the net_handler main loop terminates.
 */
void stop_dawn_conn ();

#endif