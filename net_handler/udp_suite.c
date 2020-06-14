#include "udp_suite.h"

uint16_t ports[2] = { 8200, 8201 }; //hard-coded port numbers
pthread_t td_ids[2];
td_arg_t global_td_args;

//structure of argument into a pair of udp threads
typedef struct udp_arg = {
	uint16_t send_port; //TODO: replace with the actual port data type
	uint16_t recv_port; //TODO: replace with the actual port data type
	uint8_t stop;
} udp_arg_t;

static void error (char *msg)
{
	perror(msg);
	log_runtime(ERROR, msg); //send the message to the logger
	exit(1);
}

//send DEVICE_DATA messages to Dawn
void *udp_send_thread (void *args)
{	
	udp_arg_t *udp_args = (udp_arg_t *)args; //use this to communicate between the send and recv threads
	dev_data_t dev_data;
	dev_data.msg = DEVICE_DATA;
	
	int valid_dev_ixs[MAX_DEVICES];
	dev_id_t dev_ids[MAX_DEVICES];
	uint32_t catalog, params_to_read;
	uint8_t curr_num_params;         //number of params for device currently being operated on
	int j;
	
	//TODO: figure out what happens if a device disconnects in the middle of pulling information
	while (!(udp_args->stop)) {
		//get information
		get_catalog(&catalog);
		get_device_identifiers(dev_ids);
		
		//calculate num_devices, get valid device indices
		dev_data.num_devices = 0;
		for (int i = 0, j = 0; i < MAX_DEVICES; i++) {
			if (catalog & (1 << i)) {
				dev_data.num_devices++;
				valid_dev_ixs[j++] = i;
			}
		}
		
		//populate dev_data.device_ids
		for (int i = 0; i < dev_data.num_devices; i++) {
			dev_data.device_ids[i] = dev_ids[valid_dev_ixs[i]];
		}
		
		//populate dev_data.device_params
		dev_data.device_params = (param_val_t **) malloc(sizeof(param_val_t *) * dev_data.num_devices);
		for (int i = 0; i < dev_data.num_devices; i++) {
			curr_num_params = (get_device(dev_data.device_ids[i].type))->num_params;
			dev_data.device_params[i] = (param_val_t *) malloc(sizeof(param_val_t) * curr_num_params);
			params_to_read = 0;
			for (int k = 0; k < curr_num_params; k++) {
				params_to_read |= (1 << k); //turn on the bit for every valid param
			}
			device_read(valid_dev_ixs[i], NET_HANDLER, DATA, params_to_read, &(dev_data.device_params[i])); //pull info
		}
		
		//TODO: package data and send over UDP socket
		
		//free everything
		for (int i = 0; i < dev_data.num_devices; i++) {
			free(dev_data.device_params[i]);
		}
		free(dev_data.device_params);
		
		usleep(1000); //sleep a bit so we're not looping as fast as possible
	}
	return NULL;
}

//receive GAMEPAD_STATE messages from DAWN
void *udp_recv_thread (void *args)
{
	udp_arg_t *udp_args = (udp_arg_t *)args; //use this to communicate between the send and recv threads
	
	gp_state_t gp_state;
	gp_state.msg = NOP;
	gp_state.buttons = 0;
	for (int i = 0; i < 4; i++) {
		gp_state.joystick_vals[i] = 0.0;
	}
	gamepad_write(gp_state.buttons, gp_state.joystick_values);
	robot_desc_write(GAMEPAD, CONNECTED);
	
	while (!(udp_args->stop)) {
		// TODO: receive a message from the port and load into gp_state (do NOT block here!)
		
		//handle the received packet
		if (gp_state.msg == GAMEPAD_STATE) {
			gamepad_write(gp_state.buttons, gp_state.joystick_values);
		} else if (gp_state.msg != NOP) {
			log_runtime(WARN, "unknown packet type received on UDP");
		}
		gp_state.msg = NOP;
		usleep(1000); //sleep a bit so we're not looping as fast as possible
	}
	
	gp_state.buttons = 0;
	for (int i = 0; i < 4; i++) {
		gp_state.joystick_vals[i] = 0.0;
	}
	gamepad_write(gp_state.buttons, gp_state.joystick_values);
	robot_desc_write(GAMEPAD, DISCONNECTED);
	free(udp_args); //recv thread is responsible for freeing the argument
	return NULL;
}

//start the threads managing a UDP connection
void start_udp_suite ()
{
	udp_arg_t td_args = (udp_arg_t *) malloc(sizeof(udp_arg_t));
	td_args->send_port = ports[0];
	td_args->recv_port = ports[1];
	td_args->stop = 0;
	char msg[64];     //for error messages
	
	//create send thread
	if (pthread_create(&td_ids[0], NULL, udp_send_thread, td_args) != 0) {
		sprintf(msg, "failed to create UDP send thread with DAWN_TARGET");
		error(msg);
	}
	
	//create recv thread
	if (pthread_create(&td_ids[1], NULL, udp_recv_thread, td_args) != 0) {
		sprintf(msg, "failed to create UDP recv thread with DAWN_TARGET");
		error(msg);
	}
	
	//save this for later telling the threads to stop
	global_td_args = td_args;
}

//stop the threads managing the UDP connection
void stop_udp_suite ()
{
	global_td_args.stop = 1;
	
	//join with send thread
	if (pthread_join(td_ids[0], NULL) != 0) {
		sprintf(msg, "failed to join UDP send thread with DAWN_TARGET");
		error(msg);
	}
	
	//join with recv thread
	if (pthread_join(td_ids[1], NULL) != 0) {
		sprintf(msg, "failed to join UDP recv thread with DAWN_TARGET");
		error(msg);
	}
}