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

#define RASPI_PORT 8000     //well-known port of TCP listening socket used by runtime on raspi
#define SHEPHERD_ADDR "192.168.0.25"
#define SHEPHERD_PORT 6200
#define DAWN_ADDR "192.168.0.25"
#define DAWN_PORT 7200

#define LOG_MSG_MAXLEN 512       //max length of a log message, in chars
#define MAX_NUM_LOGS 16          //maximum number of logs that can be sent in one msg
#define MAX_SIZE_BYTES 4096      //maximum number of bytes that a serialized message can be (smaller than MAX_NUM_LOGS * LOG_MSG_MAXLEN)

//All the different possible messages the network handler works with
typedef enum net_msg {
	GAMEPAD_STATE_MSG, DEVICE_DATA_MSG,           //UDP (mostly)
	RUN_MODE_MSG, CHALLENGE_DATA_MSG, LOG_MSG,    //TCP
	NOP, ACK                                      //Misc
} net_msg_t;

//the two possible endpoints
typedef enum target {
	SHEPHERD_TARGET, DAWN_TARGET
} target_t;

// ******************************************* USEFUL UTIL FUNCTIONS ******************************* //

void error (char *msg);  //TODO: consider moving this to the general runtime util

//read n bytes from fd into buf; return number of bytes read into buf
ssize_t readn (int fd, void *buf, size_t n);

//write n bytes from buf to fd; return number of bytes written to fd
ssize_t writen (int fd, const void *buf, size_t n);

//prepares send_buf by prepending msg_type into first byte, and len_pb into second and third bytes
//caller must free send_buf
void prep_buf (uint8_t *send_buf, net_msg_t msg_type, unsigned len_pb);

//reads one byte from *connfd and puts it in msg_type; reads two more bytes from *connfd
//and puts it in len_pb, then reads len_pb bytes into buf
//returns -1 if EOF read (shepherd disconnected); returns 0 on success
int parse_msg (int *connfd, net_msg_t *msg_type, uint16_t *len_pb, uint8_t *buf);

//finds max of three input numbers (used to find maxfdp1 for select)
int max (int a, int b, int c);

#endif