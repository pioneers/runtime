#include "tcp_conn.h"
#include "pbc_gen/device.pb-c.h"

//used for creating and cleaning up TCP connection
typedef struct {
	int conn_fd;
	int challenge_fd;
	int send_logs;
	FILE *log_file;
	robot_desc_field_t client;
} tcp_conn_args_t;

pthread_t dawn_tid, shepherd_tid;

/*
 * Clean up memory and file descriptors before exiting from tcp_process
 * Arguments:
 *    - void *args: should be a pointer to tcp_conn_args_t populated with the listed descriptors and pointers
 */
static void tcp_conn_cleanup (void *args)
{
	tcp_conn_args_t* tcp_args = (tcp_conn_args_t *) args;
	robot_desc_write(RUN_MODE, IDLE);

	if (close(tcp_args->conn_fd) != 0) {
		log_printf(ERROR, "Failed to close conn_fd: %s", strerror(errno));
	}
	if (close(tcp_args->challenge_fd) != 0) {
		log_printf(ERROR, "Failed to close challenge_fd: %s", strerror(errno));
	}
	if (tcp_args->log_file != NULL) {
		if (fclose(tcp_args->log_file) != 0) {
			log_printf(ERROR, "Failed to close log_file: %s", strerror(errno));
		}
		tcp_args->log_file = NULL;
	}
	robot_desc_write(tcp_args->client, DISCONNECTED);
	if (tcp_args->client == DAWN) {
		robot_desc_write(GAMEPAD, DISCONNECTED); // Disconnect gamepad if Dawn is no longer connected
	}
	free(args);
}


/* 
 * Send a log message on the TCP connection to the client. Reads lines from the pipe until there is no more data
 * or it has read MAX_NUM_LOGS lines from the pipe, packages the message, and sends it.
 * Arguments:
 *    - int conn_fd: socket connection's file descriptor on which to write to the TCP port
 */
static void send_log_msg (int conn_fd, FILE *log_file)
{	
	char nextline[MAX_LOG_LEN];   //next log line read from FIFO pipe
	Text log_msg = TEXT__INIT;    //initialize a new Text protobuf message
	log_msg.n_payload = 0;
	log_msg.payload = malloc(MAX_NUM_LOGS * sizeof(char *));
	
	//read in log lines until there are no more to read, or we read MAX_NUM_LOGS lines
	while (log_msg.n_payload < MAX_NUM_LOGS) {
		if (fgets(nextline, MAX_LOG_LEN, log_file) != NULL) {
			log_msg.payload[log_msg.n_payload] = malloc(strlen(nextline) + 1);
			strcpy(log_msg.payload[log_msg.n_payload], nextline);
			log_msg.n_payload++;
		}
		else if (errno == EAGAIN) { // EAGAIN occurs when nothing to read
			break;
		}
		else { // Error occurred
			perror("fgets");
			log_printf(ERROR, "Error reading from log fifo\n");
			return;
		}
	}
	
	//prepare the message for sending
	uint16_t len_pb = text__get_packed_size(&log_msg);
	uint8_t* send_buf = make_buf(LOG_MSG, len_pb);
	text__pack(&log_msg, send_buf + BUFFER_OFFSET);  //pack message into the rest of send_buf (starting at send_buf[3] onward)
	
	//send message on socket
	if (writen(conn_fd, send_buf, len_pb + BUFFER_OFFSET) == -1) {
		perror("writen");
		log_printf(ERROR, "sending log message failed");
	}
	
	//free all allocated memory
	for (int i = 0; i < log_msg.n_payload; i++) {
		free(log_msg.payload[i]);
	}
	free(log_msg.payload);
	free(send_buf);
}


/* 
 * Send a challenge data message on the TCP connection to the client. Reads packets from the UNIX socket from
 * executor until all messages are read, packages the message, and sends it.
 * Arguments:
 *    - int conn_fd: socket connection's file descriptor on which to write to the TCP port
 *    - int challenge_fd: Unix socket connection's file descriptor from which to read challenge results from executor
 */
static void send_challenge_results(int conn_fd, int challenge_fd) {
	// Get results from executor
	Text results = TEXT__INIT;
	results.n_payload = NUM_CHALLENGES;
	results.payload = malloc(sizeof(char*) * NUM_CHALLENGES);

	char read_buf[CHALLENGE_LEN];

	// read results from executor, line by line	
	for (int i = 0; i < NUM_CHALLENGES; i++) {
		int readlen = recvfrom(challenge_fd, read_buf, CHALLENGE_LEN, 0, NULL, NULL);
		if (readlen == CHALLENGE_LEN) {
			log_printf(WARN, "challenge_fd: Read length matches size of read buffer %d", readlen);
		}
		if (readlen < 0) { 
			perror("recvfrom");
			log_printf(ERROR, "Socket recv from challenge_fd failed");
		}
		results.payload[i] = malloc(readlen);
		strcpy(results.payload[i], read_buf);
		memset(read_buf, 0, CHALLENGE_LEN);
	}

	// Send results to client
	uint16_t len_pb = text__get_packed_size(&results);
	uint8_t* send_buf = make_buf(CHALLENGE_DATA_MSG, len_pb);
	text__pack(&results, send_buf + BUFFER_OFFSET);
	if (writen(conn_fd, send_buf, len_pb + BUFFER_OFFSET) == -1) {
		perror("write");
		log_printf(ERROR, "sending challenge data message failed");
	}

	// free allocated memory
	for (int i = 0; i < results.n_payload; i++) {
		free(results.payload[i]);
	}
	free(results.payload);
	free(send_buf);
}


/*
 * Receives new message from client on TCP connection and processes the message.
 * Arguments:
 *    - int conn_fd: socket connection's file descriptor from which to read the message
 *    - int results_fd: file descriptor of FIFO pipe to executor to which to write challenge input data if received
 * Returns: pointer to integer in which return status will be stored
 *      0 if message received and processed
 *     -1 if message could not be parsed because client disconnected and connection closed
 *     -2 if message could not be unpacked or other error
 */
static int recv_new_msg (int conn_fd, int challenge_fd)
{
	net_msg_t msg_type;           //message type
	uint16_t len_pb;              //length of incoming serialized protobuf message
	uint8_t* buf;                 //buffer to read raw data into

	int err = parse_msg(conn_fd, &msg_type, &len_pb, &buf);
	if (err == 0) { // Means there is EOF while reading which means client disconnected
		return -1; 
	}
	else if (err == -1) { // Means there is some other error while reading
		return -2; 
	}
	
	//unpack according to message
	if (msg_type == CHALLENGE_DATA_MSG) {
		//socket address structure for the UNIX socket to executor for challenge data
		struct sockaddr_un exec_addr = {0};
		exec_addr.sun_family = AF_UNIX;
		strcpy(exec_addr.sun_path, CHALLENGE_SOCKET);
		
		Text* inputs = text__unpack(NULL, len_pb, buf);
		if (inputs == NULL) {
			log_printf(ERROR, "Cannot unpack challenge_data msg");
			return -2;
		}
		if (inputs->n_payload != NUM_CHALLENGES) {
			log_printf(ERROR, "Number of challenge inputs %d is not equal to number of challenges %d", inputs->n_payload, NUM_CHALLENGES);
			return -2;
		}
		// Send to executor
		for (int i = 0; i < NUM_CHALLENGES; i++) {
			int input_len = strlen(inputs->payload[i]) + 1;
			int sendlen = sendto(challenge_fd, inputs->payload[i], input_len, 0, (struct sockaddr*) &exec_addr, sizeof(struct sockaddr_un));
			if (sendlen < 0 || sendlen != input_len) {
				perror("sendto");
				log_printf(ERROR, "Socket send to challenge_fd failed");
				return -2;
			}	
		}
		text__free_unpacked(inputs, NULL);

		log_printf(INFO, "entering CHALLENGE mode. running coding challenges!");
		robot_desc_write(RUN_MODE, CHALLENGE);
	} 
	else if (msg_type == RUN_MODE_MSG) {
		RunMode* run_mode_msg = run_mode__unpack(NULL, len_pb, buf);
		if (run_mode_msg == NULL) {
			log_printf(ERROR, "Cannot unpack run_mode msg");
			return -2;
		}
		
		//write the specified run mode to the RUN_MODE field of the robot description
		switch (run_mode_msg->mode) {
			case (MODE__IDLE):
			log_printf(INFO, "entering IDLE mode");
			robot_desc_write(RUN_MODE, IDLE);
			break;
			case (MODE__AUTO):
			log_printf(INFO, "entering AUTO mode");
			robot_desc_write(RUN_MODE, AUTO);
			break;
			case (MODE__TELEOP):
			log_printf(INFO, "entering TELEOP mode");
			robot_desc_write(RUN_MODE, TELEOP);
			break;
			case (MODE__ESTOP):
			log_printf(INFO, "ESTOP RECEIVED! entering IDLE mode");
			robot_desc_write(RUN_MODE, IDLE);
			break;
			default:
			log_printf(ERROR, "requested robot to enter invalid robot mode %s", run_mode_msg->mode);
			break;
		}
		run_mode__free_unpacked(run_mode_msg, NULL);
	} 
	else if (msg_type == START_POS_MSG) {
		StartPos* start_pos_msg = start_pos__unpack(NULL, len_pb, buf);
		if (start_pos_msg == NULL) {
			log_printf(ERROR, "Cannot unpack start_pos msg");
			return -2;
		}
		
		//write the specified start pos to the STARTPOS field of the robot description
		switch (start_pos_msg->pos) {
			case (POS__LEFT):
			log_printf(INFO, "robot is in LEFT start position");
			robot_desc_write(START_POS, LEFT);
			break;
			case (POS__RIGHT):
			log_printf(INFO, "robot is in RIGHT start position");
			robot_desc_write(START_POS, RIGHT);
			break;
			default:
			log_printf(WARN, "entered unknown start position");
			break;
		}
		start_pos__free_unpacked(start_pos_msg, NULL);
	} 
	else if (msg_type == DEVICE_DATA_MSG) {
		DevData* dev_data_msg = dev_data__unpack(NULL, len_pb, buf);
		if (dev_data_msg == NULL) {
			log_printf(ERROR, "Cannot unpack dev_data msg");
			return -2;
		}
		for (int i = 0; i < dev_data_msg->n_devices; i++) {
			Device* req_device = dev_data_msg->devices[i];
			device_t* act_device = get_device(req_device->type);
			if (act_device == NULL) {
				log_printf(ERROR, "Invalid device subscription: device type %d is invalid", req_device->type);
				continue;
			}
			uint32_t requests = 0;
			for (int j = 0; j < req_device->n_params; j++) {
				if (req_device->params[i]->val_case == PARAM__VAL_BVAL) {
					// log_printf(DEBUG, "Received device subscription for %s param %s", act_device->name, act_device->params[j].name);
					requests |= (req_device->params[j]->bval << j);
				}
			}
			int err = place_sub_request(req_device->uid, NET_HANDLER, requests);
			if (err == -1) {
				log_printf(ERROR, "Invalid device subscription: device uid %llu is invalid", req_device->uid);
			}
		}
		//TODO: tell UDP suite to only send parts of data, not implemented by Dawn yet
		
		dev_data__free_unpacked(dev_data_msg, NULL);
	}
	else {
		log_printf(WARN, "unknown message type %d, tcp should only receive CHALLENGE_DATA (2), RUN_MODE (0), START_POS (1), or DEVICE_DATA (4)", msg_type);
		return -2;
	}
	free(buf);
	return 0;
}


/*
 * Main control loop for a TCP connection. Sets up connection by opening up pipe to read log messages from
 * and sets up read_set for select(). Then it runs main control loop, using select() to make actions event-driven.
 * Arguments:
 *    - void *args: arguments containing file descriptors and file pointers that need to be closed on exit (and other settings)
 * Return:
 *    - NULL
 */
static void* tcp_process (void* tcp_args)
{
	tcp_conn_args_t* args = (tcp_conn_args_t*) tcp_args;
	pthread_cleanup_push(tcp_conn_cleanup, args);

	//variables used for waiting for something to do using select()
	fd_set read_set;
	int log_fd;
	int maxfd = args->challenge_fd > args->conn_fd ? args->challenge_fd : args->conn_fd;
	if (args->send_logs) {
		log_fd = fileno(args->log_file);
		maxfd = log_fd > maxfd ? log_fd : maxfd;
	}
	maxfd = maxfd + 1;

	//main control loop that is responsible for sending and receiving data
	while (1) {
		//set up the read_set argument to selct()
		FD_ZERO(&read_set);
		FD_SET(args->conn_fd, &read_set);
		FD_SET(args->challenge_fd, &read_set);
		if (args->send_logs) {
			FD_SET(log_fd, &read_set);
		}
		
		//prepare to accept cancellation requests over the select
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		
		//wait for something to happen
		if (select(maxfd, &read_set, NULL, NULL, NULL) < 0) {
			log_printf(ERROR, "Failed to wait for select in control loop for client %d: %s", args->client, strerror(errno));
		}
		
		//deny all cancellation requests until the next loop
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

		//send a new log message if one is available and we want to send logs
		if (args->send_logs && FD_ISSET(log_fd, &read_set)) {
			send_log_msg(args->conn_fd, args->log_file);
		}

		//send challenge results if executor sent them
		if (FD_ISSET(args->challenge_fd, &read_set)) {
			send_challenge_results(args->conn_fd, args->challenge_fd);
		}

		//receive new message on socket if it is ready
		if (FD_ISSET(args->conn_fd, &read_set)) {
			if (recv_new_msg(args->conn_fd, args->challenge_fd) == -1) { 
				log_printf(DEBUG, "client %d has disconnected", args->client);
				break;
			}
		}
	}
	
	//call the cleanup function
	pthread_cleanup_pop(1);
	return NULL;
}

/*
 * Start the main TCP connection control thread. Does not block.
 * Should be called when the client has requested a connection to Runtime.
 * Arguments:
 *    - int connfd: connection socket descriptor on which there is the established connection with the client
 */
void start_tcp_conn (robot_desc_field_t client, int conn_fd, int send_logs)
{
	//initialize argument to new connection thread
	tcp_conn_args_t* args = malloc(sizeof(tcp_conn_args_t));
	args->client = client;
	args->conn_fd = conn_fd;
	args->send_logs = send_logs;
	args->log_file = NULL;
	args->challenge_fd = -1;
	
	pthread_t* tid = (client == DAWN) ? &dawn_tid : &shepherd_tid;

	// open challenge socket to read and write
	if ((args->challenge_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		log_printf(ERROR, "failed to create challenge socket");
		return;
	}
	//set up the challenge socket
	struct sockaddr_un my_addr = {0};
	my_addr.sun_family = AF_UNIX;
	strcpy(my_addr.sun_path, CHALLENGE_SOCKET);
	
	//bind it if it doesn't exist on the system already
	if (bind(args->challenge_fd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_un)) < 0) {
		log_printf(FATAL, "challenge socket bind failed: %s", strerror(errno));
		close(args->challenge_fd);
		return;
	}

	//Open FIFO pipe for logs
	if (send_logs) {
		int log_fd;
		if ((log_fd = open(LOG_FIFO, O_RDONLY | O_NONBLOCK)) == -1) {
			perror("fifo open");
			log_printf(ERROR, "could not open log FIFO on %d", args->client);
			close(args->challenge_fd);
			return;
		}
		if ((args->log_file = fdopen(log_fd, "r")) == NULL) {
			perror("fdopen");
			log_printf(ERROR, "could not open log file from fd");
			close(args->challenge_fd);
			close(log_fd);
			return;
		}
	}

	//create the main control thread for this client
	if (pthread_create(tid, NULL, tcp_process, args) != 0) {
		perror("pthread_create");
		log_printf(ERROR, "Failed to create main TCP thread for %d", client);
		return;
	}
	robot_desc_write(client, CONNECTED);
}

/*
 * Stops the TCP connection control thread with given client cleanly. May block briefly to allow
 * main control thread to finish what it's doing. Should be called right before the net_handler main loop terminates.
 */
void stop_tcp_conn (robot_desc_field_t client)
{
	if (client != DAWN && client != SHEPHERD) {
		log_printf(ERROR, "Invalid TCP client %d", client);
		return;
	}
	
	pthread_t tid = (client == DAWN) ? dawn_tid : shepherd_tid;

	if (pthread_cancel(tid) != 0) {
		perror("pthread_cancel");
		log_printf(ERROR, "Failed to cancel TCP client thread for %d", client);
	}
	if (pthread_join(tid, NULL) != 0) {
		perror("pthread_join");
		log_printf(ERROR, "Failed to join on TCP client thread for %d", client);
	}
}
