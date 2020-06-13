#ifndef NET_HANDLER
#define NET_HANDLER

#include <czmq.h>
#include <stdio.h>

#include "../runtime_util/runtime_util.h"
#include "../shm_wrapper/shm_wrapper.h"
#include "../shm_wrapper_aux/shm_wrapper_aux.h"

#define LOG_MSG_MAXLEN 512 //max length of a log message, in chars
#define MAX_NUM_LOGS 16    //maximum number of logs that can be sent in one msg

//All the different possible messages the network handler works with
typedef enum net_msg {
	GAMEPAD_STATE, DEVICE_DATA,    //UDP
	RUN_MODE, LOG                  //log
} net_msg_t;

//the two possible endpoints
typedef enum endpoint = {
	SHEPHERD, DAWN
} endpoint_t;

//Intermediate data structures between the raw bytearray (string) sent over the network
//and the values actually written into the system:

//RUN_MODE, CODE_UPLOAD_REQUEST
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