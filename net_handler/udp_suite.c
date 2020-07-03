#include "udp_suite.h"

pthread_t socket_thread;
int socket_fd = -1;

void get_device_data(uint8_t** buffer, int* len) {
	DevData dev_data = DEV_DATA__INIT;

	dev_id_t dev_ids[MAX_DEVICES];
	int valid_dev_idxs[MAX_DEVICES];
	uint32_t catalog;
	//get information
	get_catalog(&catalog);
	get_device_identifiers(dev_ids);

	//calculate num_devices, get valid device indices
	int num_devices = 0;
	for (int i = 0, j = 0; i < MAX_DEVICES; i++) {
		if (catalog & (1 << i)) {
			num_devices++;
			valid_dev_idxs[j++] = i;
		}
	}
	dev_data.devices = malloc(num_devices * sizeof(Device *));
	log_printf(DEBUG, "Number of devices found: %d", num_devices);
	//populate dev_data.device[i]
	int dev_idx = 0;
	for (int i = 0; i < num_devices; i++) {
		int idx = valid_dev_idxs[i];
		device_t* device_info = get_device(dev_ids[idx].type);
		if (device_info == NULL) {
			log_printf(WARN, "Device %d in SHM is invalid", idx);
			continue;
		}

		dev_data.devices[dev_idx] = malloc(sizeof(Device));
		Device* device = dev_data.devices[dev_idx];
		device__init(device);
		log_printf(DEBUG, "initialized device %d", dev_idx);
		device->type = dev_ids[idx].type;
		device->uid = dev_ids[idx].uid;
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
		dev_idx++;
	}
	dev_data.n_devices = dev_idx;
	*len = dev_data__get_packed_size(&dev_data);
	log_printf(DEBUG, "Number of actual devices: %d, total size %d, DevData size %d", dev_idx, *len, sizeof(DevData));
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
	struct sockaddr_in dawn_addr;
	socklen_t addrlen = sizeof(dawn_addr);
	int size = sizeof(GpState);
	log_printf(DEBUG, "Size of GP State %d", size);
	uint8_t read_buf[size];
	int recvlen;
	uint8_t* send_buf;
	int sendlen;
	int err;

	while (1) {
		log_printf(DEBUG, "Waiting for message");
		recvlen = recvfrom(socket_fd, read_buf, size, 0, (struct sockaddr*) &dawn_addr, &addrlen);
		log_printf(DEBUG, "Receive size %d", recvlen);
		if (recvlen < 0) {
			perror("recvfrom");
			log_printf(ERROR, "UDP recvfrom failed");
		}
		log_printf(DEBUG, "Dawn IP is %s", inet_ntoa(dawn_addr.sin_addr));
		update_gamepad_state(read_buf, recvlen);
		log_printf(DEBUG, "Updated gamepad. Getting device data");
		get_device_data(&send_buf, &sendlen);
		log_printf(DEBUG, "sending dev data to Dawn");
		err = sendto(socket_fd, send_buf, sendlen, 0, (struct sockaddr*) &dawn_addr, addrlen);
		if (err <= 0) {
			perror("sendto");
			log_printf(ERROR, "UDP sendto failed");
		}
		log_printf(DEBUG, "send buffer length %d, actual sent %d", sendlen, err);
		free(send_buf);  // Free buffer with device data protobuf
		addrlen = sizeof(dawn_addr);
	}
	return NULL;
}


//start the threads managing a UDP connection
void start_udp_conn ()
{
	struct sockaddr_in my_addr;    //for holding IP addresses (IPv4)
	
	//create the socket
	if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("could not create udp socket");
		return;
	}
	
	//bind the socket to any valid IP address on the raspi and specified port
	memset(&my_addr, 0, sizeof(struct sockaddr_in));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin_port = htons(UDP_PORT);
	if (bind(socket_fd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) < 0) {
		perror("udp socket bind failed");
		return;
	}
	
	//create send thread
	if (pthread_create(&socket_thread, NULL, process_udp_data, NULL) != 0) {
		perror("pthread_create");
		log_printf(ERROR, "failed to create UDP thread");
	}
	
}

//stop the threads managing the UDP connection
void stop_udp_conn ()
{
	if (pthread_cancel(socket_thread) != 0) {
		perror("pthread_cancel");
		log_printf(ERROR, "failed to cancel UDP thread");
	}
	if (pthread_join(socket_thread, NULL) != 0) {
		perror("pthread_join");
		log_printf(ERROR, "failed to join UDP thread");
	}
	if(close(socket_fd) != 0) {
		perror("close");
		log_printf(ERROR, "Couldn't close UDP socket properly");
	}
}
