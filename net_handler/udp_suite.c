#include "udp_suite.h"
// #include "udp_lib.c"

#define UDP_BUF_SIZE 2000   //largest size, in bytes, of a message (dev_ids could be pretty big)
#define UDP_PORT 8192       //arbitrarily chosen port number (2^13 bc this was written year 13)

//structure of argument into a pair of udp threads
typedef struct {
	int socket;                       //socket descriptor (returned by socket())
	struct sockaddr_in* dawn_addr;   //description of dawn endpoint
	uint8_t stop;					//signal to stop socket
} udp_args_t;

pthread_t socket_thread;
udp_args_t thread_args;


// ******************************************* CORE FUNCTIONS *********************************** //

void get_device_data(uint8_t** buffer, uint32_t* len) {
	DevData dev_data = DEV_DATA__INIT;

	dev_id_t dev_ids[MAX_DEVICES];
	int valid_dev_idxs[MAX_DEVICES];
	uint32_t catalog, params_to_read;
	//get information
	get_catalog(&catalog);
	get_device_identifiers(dev_ids);

	//calculate num_devices, get valid device indices
	dev_data.n_devices = 0;
	for (int i = 0, j = 0; i < MAX_DEVICES; i++) {
		if (catalog & (1 << i)) {
			dev_data.n_devices++;
			valid_dev_idxs[j++] = i;
		}
	}
	dev_data.devices = malloc(dev_data.n_devices * sizeof(Device *));
	log_printf(DEBUG, "Number of devices found: %d", dev_data.n_devices);
	//populate dev_data.device[i]
	for (int i = 0; i < dev_data.n_devices; i++) {
		dev_data.devices[i] = malloc(sizeof(Device));
		Device* device = dev_data.devices[i];
		device__init(device);
		log_printf(DEBUG, "initialized device %d", i);
		int idx = valid_dev_idxs[i];
		device->type = dev_ids[idx].type;
		device->uid = dev_ids[idx].uid;
		device_t* device_info = get_device(dev_ids[idx].type);
		if (device_info == NULL) {
			log_printf(WARN, "Device %d in SHM is invalid", idx);
			continue;
		}
		device->name = device_info->name;

		device->n_params = device_info->num_params;
		param_val_t param_data[MAX_PARAMS];
		uint32_t params_to_read = 0;
		for (int k = 0; k < device->n_params; k++)
			params_to_read |= (1 << k);
		device_read_uid(device->uid, NET_HANDLER, DATA, params_to_read, param_data);

		device->params = malloc(device->n_params * sizeof(Param*));
		//populate device parameters
		for (int j = 0; j < device->n_params; j++) {
			device->params[j] = malloc(sizeof(Param));
			Param* param = device->params[j];
			param__init(param);
			param->name = device_info->params[j].name;
			if(strcmp(device_info->params[j].type, "int") == 0) {
				param->val_case = PARAM__VAL_IVAL;
				param->ival = param_data[j].p_i;
			}
			else if (strcmp(device_info->params[j].type, "float") == 0) {
				param->val_case = PARAM__VAL_FVAL;
				param->fval = param_data[j].p_f;
			}
			else if(strcmp(device_info->params[j].type, "bool") == 0) {
				param->val_case = PARAM__VAL_BVAL;
				param->bval = param_data[j].p_b;
			}
		}
	}

	*len = dev_data__get_packed_size(&dev_data);
	*buffer = malloc(*len);
	dev_data__pack(&dev_data, *buffer);

	//free everything
	for (int i = 0; i < dev_data.n_devices; i++) {
		for (int j = 0; j < dev_data.devices[i]->n_params; j++) {
			free(dev_data.devices[i]->params[j]);
		}
		free(dev_data.devices[i]->params);
		free(dev_data.devices[i]);
	}
	free(dev_data.devices);
}


void update_gamepad_state(uint8_t* buffer, int len) {
	GpState* gp_state = gp_state__unpack(NULL, len, buffer);
	if (gp_state->n_axes != 4) {
		log_printf(ERROR, "Number of joystick axes given is %d which is not 4. Cannot update gamepad state", gp_state->n_axes);
	}
	else {
		// display the message's fields.
		log_printf(DEBUG, "Is gamepad connected: %d. Received: buttons = %d\n\taxes:", gp_state->connected, gp_state->buttons);
		for (int i = 0; i < gp_state->n_axes; i++) {
			log_printf(PYTHON, "\t%f", gp_state->axes[i]);
		}
		log_printf(PYTHON, "\n");
		robot_desc_write(GAMEPAD,  gp_state->connected ? CONNECTED : DISCONNECTED);
		if (gp_state->connected) {
			gamepad_write(gp_state->buttons, gp_state->axes);
		}
	}
	gp_state__free_unpacked(gp_state, NULL);
}


void* process_udp_data(void* args) {
	udp_args_t* thread_args = (udp_args_t*) args;
	DevData dev_data = DEV_DATA__INIT;
	struct sockaddr_in* dawn_addr;
	int first = 1;
	socklen_t addrlen = sizeof(*dawn_addr);
	int initial_len = addrlen;
	int size = sizeof(GpState);
	uint8_t read_buf[size];
	int recvlen;
	uint8_t* send_buf;
	int sendlen;
	int err;

	while (!thread_args->stop) {
		if (first) { 
			log_printf(DEBUG, "Waiting for first message");
			recvlen = recvfrom(thread_args->socket, read_buf, size, 0, (struct sockaddr*) dawn_addr, &addrlen);
			log_printf(DEBUG, "Dawn IP is %s", inet_ntoa(dawn_addr->sin_addr));
			first = 0;
		}
		else {
			log_printf(DEBUG, "Waiting for message");
			recvlen = recvfrom(thread_args->socket, read_buf, size, 0, NULL, NULL);
		}
		if (recvlen <= 0) {
			log_printf(ERROR, "UDP recvfrom failed. First connect %d", dawn_addr == NULL);
			perror("recvfrom");
		}
		log_printf(DEBUG, "Received data from Dawn");
		update_gamepad_state(read_buf, recvlen);
		log_printf(DEBUG, "Updated gamepad. Getting device data");
		get_device_data(&send_buf, &sendlen);
		log_printf(DEBUG, "sending dev data to Dawn");
		err = sendto(thread_args->socket, send_buf, sendlen, 0, (struct sockaddr*) dawn_addr, addrlen);
		if (err <= 0) {
			log_printf(ERROR, "UDP sendto failed");
			perror("sendto");
		}
		free(send_buf);
	}

}


//start the threads managing a UDP connection
void start_udp_suite ()
{
	struct sockaddr_in my_addr;    //for holding IP addresses (IPv4)
	thread_args.stop = 0;
	int ready = 0;
	
	//create the socket
	if ((thread_args.socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("could not create udp socket");
		return;
	}
	
	//bind the socket to any valid IP address on the raspi and specified port
	memset(&my_addr, 0, sizeof(struct sockaddr_in));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin_port = htons(UDP_PORT);
	if (bind(thread_args.socket, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) < 0) {
		perror("udp socket bind failed");
		return;
	}
	
	//create send thread
	if (pthread_create(&socket_thread, NULL, process_udp_data, &thread_args) != 0) {
		perror("pthread_create");
		log_printf(ERROR, "failed to create UDP thread");
	}
	
}

//stop the threads managing the UDP connection
void stop_udp_suite ()
{
	thread_args.stop = 1;
	
	//join with send thread
	if (pthread_join(socket_thread, NULL) != 0) {
		log_printf(ERROR, "failed to join UDP thread with DAWN_TARGET");
	}
	
	//close the sockets
	close(thread_args.socket);
}


void sigintHandler(int signum) {
	stop_udp_suite();
}


int main() {
	// signal(SIGINT, sigintHandler);
	logger_init(NET_HANDLER);
	shm_aux_init(NET_HANDLER);
	shm_init(NET_HANDLER);
	start_udp_suite();
	
	while (1) {}
}
