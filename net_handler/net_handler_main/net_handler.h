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

//Intermediate data structures between the raw bytearray (string) sent over the network
//and the values actually written into the system:

//RUN_MODE, CHALLENGE_DATA
struct text_payload = {
	net_msg_t msg;
	char *text;
} text_payload_t;

//LOG
struct log_payload = {
	net_msg_t msg;
	int num_logs;
	char logs[MAX_NUM_LOGS][LOGS_MSG_MAXLEN];     //array of strings, one line per string
} log_payload_t;

//DEVICE_DATA
struct dev_data = {
	net_msg_t msg;
	int num_devices;
	dev_id_t *device_ids;
	param_val_t **device_params;  // array of devices of params
} dev_data_t;

//GAMEPAD_STATE
struct gp_state = {
	net_msg_t msg;
	uint32_t buttons;
	float joystick_values[4];
} gp_state_t;

#endif