#ifndef NET_HANDLER
#define NET_HANDLER

#include <stdio.h>
#include <stdlib.h>      //for malloc, free, exit
#include <string.h>      //for strcpy, memset
#include <arpa/inet.h>   //for inet_addr, bind, listen, accept, socket types
#include <unistd.h>      //for read, write, close

#include "../runtime_util/runtime_util.h"
#include "../shm_wrapper/shm_wrapper.h"
#include "../shm_wrapper_aux/shm_wrapper_aux.h"

#define RASPI_PORT 8000     //well-known port of TCP listening socket used by runtime on raspi
#define SHEPHERD_ADDR "127.0.0.1"
#define SHEPHERD_PORT 6200
#define DAWN_ADDR "127.0.0.1"
#define DAWN_PORT 7200

#define LOG_MSG_MAXLEN 512 //max length of a log message, in chars
#define MAX_NUM_LOGS 16    //maximum number of logs that can be sent in one msg

//All the different possible messages the network handler works with
typedef enum net_msg {
	GAMEPAD_STATE, DEVICE_DATA,       //UDP (mostly)
	RUN_MODE, CHALLENGE_DATA, LOG,    //TCP
	NOP, ACK                          //Misc
} net_msg_t;

//the two possible endpoints
typedef enum target = {
	SHEPHERD_TARGET, DAWN_TARGET
} target_t;


#endif