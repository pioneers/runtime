#include "udp_suite.h"

#define UDP_BUF_SIZE 2000   //largest size, in bytes, of a message (dev_ids could be pretty big)
#define UDP_PORT 8192       //arbitrarily chosen port number (2^13 bc this was written year 13)

pthread_t td_ids[2];
td_arg_t global_td_args;

//structure of argument into a pair of udp threads
typedef struct udp_arg = {
	int sock;                       //socket descriptor (returned by socket())
	struct sockaddr_in dawn_addr;   //description of dawn endpoint
	uint8_t stop;
} udp_arg_t;

// ****************************************** HELPER FUNCTIONS ********************************** //

static void error (char *msg)
{
	perror(msg);
	log_runtime(ERROR, msg); //send the message to the logger
}

//send given dev_data message on given socket
static void send_dev_data (udp_arg_t *udp_args, dev_data_t *dev_data)
{
	char buf[UDP_BUF_SIZE];
	
	//flatten the dev_data struct and paste it piece by piece into buf
	//find the length of the final string
	//send it with sendto()
}

//wait for a message from Dawn, and paste contents into provided struct
static void recv_gp_stae (udp_arg_t *udp_args, gp_state_t *gp_state)
{
	char buf[UDP_BUF_SIZE];
	ssize_t bytes_read;
	
	if ((bytes_read = recvfrom(udp_args->socket, buf, UDP_BUF_SIZE, 0, NULL, sizeof(struct sockaddr_in))) <= 0) {
		error("recvfrom failed in main loop");
	}
	
	if (bytes_read <= sizeof(gp_state_t)) {
		memcpy(gp_state, (const void *) buf, (size_t) bytes_read); //paste all the bytes read from buf to gp_state
	} else {
		memcpy(gp_state, (const void *) buf, sizeof(gp_state_t)); //otherwise paste sizeof(gp_state_t) bytes so gp_state_t doesn't overflow
	}
}

// ******************************************* CORE FUNCTIONS *********************************** //

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
		
		//package data and send over socket
		send_dev_data(udp_args, &dev_data);
		
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
		recv_gp_state(udp_args, &gp_state);
		
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
	struct sockaddr_in my_addr, dawn_addr;    //for holding IP addresses (IPv4)
	td_args->stop = 0;
	char msg[64];        //for error messages
	char buf[256];       //buffer for receiving messages
	int ready = 0;
	
	//create the socket
	if ((td_args->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		error("could not create udp socket");
		return;
	}
	
	//bind the socket to any valid IP address on the raspi and specified port
	memset((char *)&my_addr, 0, sizeof(struct sockaddr_in));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin_port = htons(PORT);
	if (bind(td_args->sock, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))) < 0) {
		error("udp socket bind failed");
		free(td_args);
		fclose(&(td_args->socket));
		return;
	}
	
	//get the address of Dawn instance asking for connection
	while (!ready) {
		if (recvfrom(td_args->socket, buf, 256, 0, (struct sockaddr *)&dawn_addr, sizeof(struct sockaddr_in)) <= 0) {
			error("failed to receive coherent message from Dawn");
		}
		if ((net_msg_t) buf[0] == ACK) {
			td_args->dawn_addr = dawn_addr;
			//TODO: what happens if this fails
			if (sendto(td_args->socket, buf, 1, 0, (struct sockaddr *)&dawn_addr, sizeof(struct sockaddr_in)) <= 0) {
				error("failed to send acknowledgement to Dawn");
			}
			ready = 1;
		}
	}
	
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
	
	//close the sockets
	fclose(&(global_td_args.send_fd));
	fclose(&(global_td_args.recv_fd));
}