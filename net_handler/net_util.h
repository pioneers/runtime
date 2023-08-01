#ifndef NET_UTIL
#define NET_UTIL

#include <arpa/inet.h>   //for inet_addr, bind, listen, accept, socket types
#include <netinet/in.h>  //for structures relating to IPv4 addresses
#include <pthread.h>     //for threading
#include <signal.h>      //for signal
#include <stdbool.h>     // for booleans
#include <stdio.h>
#include <stdlib.h>  //for malloc, free, exit
#include <string.h>  //for strcpy, memset
#include <sys/un.h>  //for unix sockets
#include <unistd.h>  //for read, write, close

// include other runtime files
#include <logger.h>
#include <runtime_util.h>
#include <shm_wrapper.h>

// include compiled protobuf headers
#include <device.pb-c.h>
#include <gamestate.pb-c.h>
#include <input.pb-c.h>
#include <run_mode.pb-c.h>
#include <start_pos.pb-c.h>
#include <text.pb-c.h>
#include <timestamp.pb-c.h>

#define RASPI_ADDR "127.0.0.1"  // The IP address of Runtime (Raspberry Pi) that clients can request a connection to
#define RASPI_TCP_PORT 8101     // Port for Runtime as a TCP socket server

#define MAX_NUM_LOGS 16  // Maximum number of logs that can be sent in one msg

#define BUFFER_OFFSET 3  // Num bytes at the beginning of a buffer for metadata (message type and length) See net_util::make_buf()

// All the different possible messages the network handler works with. The order must be the same between net_handler and clients
typedef enum net_msg {
    RUN_MODE_MSG,
    START_POS_MSG,
    LOG_MSG,
    DEVICE_DATA_MSG,
    GAME_STATE_MSG,
    INPUTS_MSG,  // used for converter testing; remove after 2021 Spring Comp...maybe
    TIME_STAMP_MSG
} net_msg_t;

// ******************************************* USEFUL UTIL FUNCTIONS ******************************* //

/*
 * Prepares a buffer of uint8_t for receiving a packed protobuf message of the specified type and length.
 * The prepared buffer must be freed by the caller.
 * Arguments:
 *    - net_msg_t msg_type: one of the message types defined in net_util.h
 *    - unsigned len_pb: length of the serialized bytes returned by the protobuf function *__get_packed_size()
 * Return:
 *    - pointer to uint8_t that was malloc'ed, with the first BUFFER_OFFSET bytes set appropriately and with exactly enough space
 *      to fit the rest of the serialized message into
 */
uint8_t* make_buf(net_msg_t msg_type, uint16_t len_pb);

/*
 * Parses a message from the given file descriptor into its separate components and stores them in provided pointers
 * Arguments:
 *    - int *fd: pointer to file descriptor from which to read the incoming message
 *    - net_msg_t *msg_type: message type of the incoming message will be stored in this location upon successful return
 *    - uint16_t *len_pb: serialized length, in bytes, of the incoming message will be stored in this location upon successful return
 *    - uint8_t *buf: serialized message will be stored starting at this location upon successful return
 * Return:
 *    - 0: successful return
 *    - -1: Error/EOF encountered when reading from fd
 */
int parse_msg(int fd, net_msg_t* msg_type, uint16_t* len_pb, uint8_t** buf);

#endif
