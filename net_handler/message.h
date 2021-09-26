#include "net_util.h"

/*
 * Send a log message on the TCP connection to the client. Reads lines from the pipe until there is no more data
 * or it has read MAX_NUM_LOGS lines from the pipe, packages the message, and sends it.
 * Arguments:
 *    - int conn_fd: socket connection's file descriptor on which to write to the TCP port
 */
void send_log_msg(int conn_fd, FILE* log_file);

/*
* Send a timestamp message over TCP with a timestamp attached to the message. The timestamp is when the net_handler on runtime has received the
* packet and processed it. 
* Arguments:
    - int conn_fd: socket connection's file descriptor on which to write to the TCP port
    - TimeStamps* dawn_timestamp_msg: Unpacked timestamp_proto from Dawn
*/
void send_timestamp_msg(int conn_fd, TimeStamps* dawn_timestamp_msg);

/**
 * Sends a Device Data message to Dawn.
 * Arguments:
 *    - int dawn_socket_fd: socket fd for Dawn connection
 *    - uint64_t dawn_start_time: time that Dawn connection started, for calculating time in CustomData
 */
void send_device_data(int dawn_socket_fd, uint64_t dawn_start_time);

/*
 * Receives new message from client on TCP connection and processes the message.
 * Arguments:
 *    - int conn_fd: socket connection's file descriptor from which to read the message
 *    - robot_desc_field_t client: DAWN or SHEPHERD, depending on which connection is being handled
 * Returns: pointer to integer in which return status will be stored
 *      0 if message received and processed
 *     -1 if message could not be parsed because client disconnected and connection closed
 *     -2 if message could not be unpacked or other error
 */
int recv_new_msg(int conn_fd, robot_desc_field_t client);