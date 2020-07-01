#ifndef NET_UTIL
#define NET_UTIL

#include <stdio.h>
#include <stdlib.h>      //for malloc, free, exit
#include <string.h>      //for strcpy, memset
#include <arpa/inet.h>   //for inet_addr, bind, listen, accept, socket types
#include <netinet/in.h>  //for structures relating to IPv4 addresses
#include <unistd.h>      //for read, write, close
#include <pthread.h>     //for threading
#include <signal.h> 	 //for signal

//include other runtime files
#include "../runtime_util/runtime_util.h"
#include "../shm_wrapper/shm_wrapper.h"
#include "../shm_wrapper_aux/shm_wrapper_aux.h"
#include "../logger/logger.h"

//include compiled protobuf headers
#include "pbc_gen/device.pb-c.h"
#include "pbc_gen/gamepad.pb-c.h"
#include "pbc_gen/run_mode.pb-c.h"
#include "pbc_gen/text.pb-c.h"
#include "pbc_gen/start_pos.pb-c.h"

#define RASPI_PORT 8101     //well-known port of TCP listening socket used by runtime on raspi
#define SHEPHERD_ADDR "127.0.0.1"
#define SHEPHERD_PORT 6101
#define DAWN_ADDR "127.0.0.1"
#define DAWN_PORT 7101

#define UDP_PORT 9000

#define MAX_NUM_LOGS 16          //maximum number of logs that can be sent in one msg
#define MAX_SIZE_BYTES 4096      //maximum number of bytes that a serialized message can be (smaller than MAX_NUM_LOGS * LOG_MSG_MAXLEN)

//All the different possible messages the network handler works with
typedef enum net_msg {
	RUN_MODE_MSG, START_POS_MSG, CHALLENGE_DATA_MSG, LOG_MSG, DEVICE_DATA_MSG
} net_msg_t;

// ******************************************* USEFUL UTIL FUNCTIONS ******************************* //


/*
 * Read n bytes from fd into buf; return number of bytes read into buf (deals with interrupts and unbuffered reads)
 * Arguments:
 *    - int fd: file descriptor to read from
 *    - void *buf: pointer to location to copy read bytes into
 *    - size_t n: number of bytes to read
 * Return:
 *    - > 0: number of bytes read into buf
 *    - 0: read EOF on fd
 *    - -1: read errored out
 */
ssize_t readn (int fd, void *buf, size_t n);

/*
 * Read n bytes from buf to fd; return number of bytes written to buf (deals with interrupts and unbuffered writes)
 * Arguments:
 *    - int fd: file descriptor to write to
 *    - void *buf: pointer to location to read from
 *    - size_t n: number of bytes to write
 * Return:
 *    - >= 0: number of bytes written into buf
 *    - -1: write errored out
 */
ssize_t writen (int fd, const void *buf, size_t n);

/*
 * Prepares a buffer of uint8_t for receiving a packed protobuf message of the specified type and length.
 * Also converts the specified length of message from unsigned to uint16_t and returns it in the third argument.
 * Returns the prepared buffer containing the message type in the first element and the length in the second and third elements.
 * The prepared buffer must be freed by the caller.
 * Arguments:
 *    - net_msg_t msg_type: one of the message types defined in net_util.h
 *    - unsigned len_pb: length of the serialized bytes returned by the protobuf function *__get_packed_size() 
 *    - uint16_t *len_pb_uint16: upon successful return, will hold the given len_pb as a uint16_t (useful for pointer arithmetic)
 * Return:
 *    - pointer to uint8_t that was malloc'ed, with the first three bytes set appropriately and with exactly enough space
 *      to fit the rest of the serialized message into
 */
uint8_t *prep_buf (net_msg_t msg_type, int len_pb);

/*
 * Parses a message from the given file descriptor into its separate components and stores them in provided pointers
 * Arguments:
 *    - int *fd: pointer to file descriptor from which to read the incoming message
 *    - net_msg_t *msg_type: message type of the incoming message will be stored in this location upon successful return
 *    - uint16_t *len_pb: serialized length, in bytes, of the incoming message will be stored in this location upon successful return
 *    - uint8_t *buf: serialized message will be stored starting at this location upon successful return
 * Return:
 *    - 0: successful return
 *    - -1: EOF encountered when reading from fd
 */
int parse_msg (int fd, net_msg_t *msg_type, uint16_t *len_pb, uint8_t** buf);

#endif
