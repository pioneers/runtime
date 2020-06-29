#include "tcp_conn.h"

//used for creating and cleaning up TCP connection
typedef struct {
	int conn_fd;
    int send_logs;
    robot_desc_val_t client;
} tcp_conn_args_t;

pthread_t dawn_tid, shepherd_tid;
int log_fd;
int challenge_fd;

//for the internal log message buffering
char glob_buf[MAX_LOG_LEN];
int glob_buf_ix = 0;  //index of glob_buf we are currently operating on
int glob_buf_end = 0; //index of first invalid char in glob_buf

/*
 * Clean up memory and file descriptors before exiting from dawn connection main control loop
 * Arguments:
 *    - void *args: should be a pointer to cleanup_objs_t populated with the listed descriptors and pointers
 */
static void tcp_conn_cleanup (void *args)
{
	tcp_conn_args_t* tcp_args = (tcp_conn_args_t *) args;

	if (close(log_fd) != 0) {
        log_printf(ERROR, "Failed to close log_fd");
		perror("close: log_fd");
	}
	if (close(tcp_args->conn_fd) != 0) {
        log_printf(ERROR, "Failed to close conn_fd");
		perror("close: connfd");
	}
    // if (close(challenge_fd) != 0) {
    //     log_printf(ERROR, "Failed to close challenge_fd");
    //     perror("close: challenge_fd");
    // }
	robot_desc_write(tcp_args->client, DISCONNECTED);
	free(args);
}

/* 
 * Read the next line of text from FIFO, including a terminating newline and null character, into a string.
 * The returned string is exactly long enough to fit the text, and needs to be freed by the caller.
 * Arguments:
 *    - int fifofd: file descriptor of the log pipe to read from
 *    - int *retval: pointer to integer to save a status message in
 *            - *retval = 1 is set if no more could be read from pipe because there is no data
 *            - *retval = 0 is set if EOF read from pipe (all writers have closed)
 *            - *retval = -1 is set if read failed in an unexpected way
 *            - *retval = 2 if read successful
 * Return:
 *    - pointer to character (string) that is newline- and null-terminated containing next line from pipe
 *    - NULL if error (check retval for specific value)
 */
static char *readline (int fifofd, int *retval)
{
	static char buf[MAX_LOG_LEN];
	char *line;
	ssize_t n_read;
	int i = 0;
	
	//if there is still stuff left to read in glob_buf
	if (glob_buf_ix < glob_buf_end) {
		//printf("copying remaining contents before read\n");
		while (glob_buf_ix < glob_buf_end && glob_buf[glob_buf_ix] != '\n') {
			buf[i++] = glob_buf[glob_buf_ix++];
		}
	}
	log_printf(DEBUG, "About to read %d from queue", MAX_LOG_LEN);
	//if we got to the end of glob_buf, we need to read more from pipe
	if (glob_buf_ix == glob_buf_end) {
		log_printf(DEBUG, "Reading from %d", fifofd);
		scanf("test");
		n_read = read(fifofd, glob_buf, MAX_LOG_LEN);
		//handle error cases and return NULL
		if (n_read == -1) {
			perror("read");
			if (errno == EAGAIN || errno == EWOULDBLOCK) { //if no more data on fifo pipe
				*retval = 1;
			} else { //some other error
				*retval = -1;
				log_printf(ERROR, "log fifo read failed");
			}
			return NULL;
		} else if (n_read == 0) { //EOF
			*retval = 0;
			perror("read EOF");
			return NULL;
		}
		
		//set global vars
		glob_buf_ix = 0;
		glob_buf_end = n_read;
		
		//copy from glob_buf into buf until newline
		while (glob_buf[glob_buf_ix] != '\n') {
			buf[i++] = glob_buf[glob_buf_ix++];
		}
	}
	glob_buf_ix++; //glob_buf_ix needs to point to next uncopied character after the newline
	
	//finished reading next line into buf, now copy into line and newline- and null-terminate it
	line = (char *) malloc(sizeof(char) * (i + 2)); //+2 for the newline and null characters
	memcpy(line, buf, i);
	line[i] = '\n';
	line[i + 1] = '\0';
	*retval = 2;
	
	return line;
}

/* 
 * Send a log message on the TCP connection to the client. Reads lines from the pipe until there is no more data
 * or it has read MAX_NUM_LOGS lines from the pipe, packages the message, and sends it.
 * Arguments:
 *    - int *connfd: pointer to connection socket descriptor on which to write to the TCP port
 *    - int fifofd: file descriptor of FIFO pipe from which to read log lines
 *    - Text *log_msg: pointer to Text protobuf message structure in which to build the log message
 * Return:
 *    - 1: log message sent successfully
 *    - 0: EOF encoutered on pipe; no more writers to the pipe
 *    - -1: pipe errored out on read
 */
static int send_log_msg (int connfd, int fifofd)
{	
	char *nextline;            //next log line read from FIFO pipe
	int retval;                //return value for readline()
	
	//read in log lines until there are no more to read, or we read MAX_NUM_LOGS lines
    Text log_msg = TEXT__INIT;
	log_msg.n_payload = 0;
	log_printf(DEBUG, "Starting to read from queue");
	while (log_msg.n_payload < MAX_NUM_LOGS) {
		if ((nextline = readline(fifofd, &retval)) == NULL) {
			if (retval == 1) { //if no more data to read (EWOULDBLOCK or EAGAIN)
				break;
			} else if (retval == 0) {  //if EOF (all writers of FIFO pipe crashed; shouldn't happen)
				log_printf(DEBUG, "EOF in readline");
				return 0;
			} else if  (retval == -1) {  //some other error
				log_printf(DEBUG, "Some other error in readline");
				return -1;
			}
		}
		log_msg.payload[log_msg.n_payload] = nextline;
		log_msg.n_payload++;
	}
	log_printf(ERROR, "read lines from FIFO");
	//prepare the message for sending
	int len_pb = text__get_packed_size(&log_msg);
	uint8_t* send_buf = prep_buf(LOG_MSG, len_pb);
	text__pack(&log_msg, (void *) (send_buf + 3));  //pack message into the rest of send_buf (starting at send_buf[3] onward)
	
	//send message on socket
	if (writen(connfd, send_buf, len_pb + 3) == -1) {
		perror("writen");
		log_printf(ERROR, "sending log message failed");
	}
	
	//free all allocated memory
	for (int i = 0; i < log_msg.n_payload; i++) {
		free(log_msg.payload[i]);
	}
	free(send_buf);
	return 1;
}


/* 
 * Send a challenge data message on the TCP connection to the client.
 * Arguments:
 *    - int *connfd: pointer to connection socket descriptor on which to write to the TCP port
 *    - int results_fd: file descriptor of FIFO pipe from which to read challenge results from executor
 *    - Text *challenge_data_msg: pointer to Text protobuf message structure in which to build the challenge data message
 */
static void send_challenge_msg (int connfd, int results_fd)
{
	const int max_len = 1000;   //max len of a challenge data message
	size_t result_len;          //length of results string
	char buf[max_len];          //buffer that results are read into, from which it is copied into text message
	
	//keep trying to read the line until you get it; copy that line into challenge_data_msg.payload[0]
	//TODO: change this to deal with FIFO pipe
	//while (read(buf, max_len, results_fd) != 0);
	result_len = strlen((const char *) buf);
    Text challenge_data_msg = TEXT__INIT;
	challenge_data_msg.payload[0] = malloc(result_len);
	strcpy(challenge_data_msg.payload[0], (const char *) buf);
	
	//prepare the message for sending
	int len_pb = text__get_packed_size(&challenge_data_msg);
	uint8_t* send_buf = prep_buf(CHALLENGE_DATA_MSG, len_pb);
	text__pack(&challenge_data_msg, (void *) (send_buf + 3));  //pack message into the rest of send_buf (starting at send_buf[3] onward)
	
	//send message on socket
	if (writen(connfd, send_buf, len_pb + 3) == -1) {
		perror("write");
        log_printf(ERROR, "sending challenge data message failed");
	}
	
	//free all allocated memory
	free(challenge_data_msg.payload[0]);
	free(send_buf);
}


/*
 * Receives new message from client on TCP connection and processes the message.
 * Arguments:
 *    - int *connfd: pointer to connection socket descriptor from which to read the message
 *    - int results_fd: file descriptor of FIFO pipe to executor to which to write challenge input data if received
 * Returns: pointer to integer in which return status will be stored
 *      0 if message received and processed
 *     -1 if message could not be parsed because Shepherd disconnected and connection closed
 */
static int recv_new_msg (int connfd, int results_fd)
{
	net_msg_t msg_type;           //message type
	uint16_t len_pb;              //length of incoming serialized protobuf message
	uint8_t* buf;                 //buffer to read raw data into
	size_t challenge_len;         //length of the challenge data received string
	
	//get the msg type and length in bytes; read the rest of msg into buf according to len_pb
	//if parse_msg doesn't return 0, we found EOF and shepherd disconnected
	if (parse_msg(connfd, &msg_type, &len_pb, buf) != 0) {
		log_printf(ERROR, "Cannot parse message");
		return -1;
	};
	
	//unpack according to message
	if (msg_type == CHALLENGE_DATA_MSG) {
		log_printf(DEBUG, "entering CHALLENGE mode. running coding challenges!");
		robot_desc_write(RUN_MODE, CHALLENGE);

		Text* challenge_data_msg = text__unpack(NULL, len_pb, buf);
		//TODO: write the provided input data for the challenge to FIFO pipe to executor
		challenge_len = strlen(challenge_data_msg->payload[0]);
		if (writen(results_fd, challenge_data_msg->payload[0], challenge_len) == -1) {
			perror("fwrite");
			log_printf(ERROR, "could not write to results.txt for challenge data");
		}
		text__free_unpacked(challenge_data_msg, NULL);
	} 
	else if (msg_type == RUN_MODE_MSG) {
		RunMode* run_mode_msg = run_mode__unpack(NULL, len_pb, buf);
		
		//write the specified run mode to the RUN_MODE field of the robot description
		switch (run_mode_msg->mode) {
			case (MODE__IDLE):
				log_printf(DEBUG, "entering IDLE mode");
				robot_desc_write(RUN_MODE, IDLE);
				break;
			case (MODE__AUTO):
				log_printf(DEBUG, "entering AUTO mode");
				robot_desc_write(RUN_MODE, AUTO);
				break;
			case (MODE__TELEOP):
				log_printf(DEBUG, "entering TELEOP mode");
				robot_desc_write(RUN_MODE, TELEOP);
				break;
			case (MODE__ESTOP):
				log_printf(DEBUG, "ESTOP RECEIVED! entering IDLE mode");
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
		
		//write the specified start pos to the STARTPOS field of the robot description
		switch (start_pos_msg->pos) {
			case (POS__LEFT):
				log_printf(DEBUG, "robot is in LEFT start position");
				robot_desc_write(START_POS, LEFT);
				break;
			case (POS__RIGHT):
				log_printf(DEBUG, "robot is in RIGHT start position");
				robot_desc_write(START_POS, RIGHT);
				break;
			default:
				log_printf(WARN, "entered unknown start position");
		}
		start_pos__free_unpacked(start_pos_msg, NULL);
	} 
    else if (msg_type == DEVICE_DATA_MSG) {
		DevData* dev_data_msg = dev_data__unpack(NULL, len_pb, buf);
		log_printf(WARN, "Received device data msg which has no implementation");
		//TODO: tell UDP suite to only send parts of data
		
		dev_data__free_unpacked(dev_data_msg, NULL);
	}
	else {
		log_printf(WARN, "unknown message type %d; shepherd should only send RUN_MODE (2), START_POS (3), or CHALLENGE_DATA (4)", msg_type);
	}
	free(buf);
	return 0;
}


/*
 * Main control loop for dawn connection. Sets up connection by opening up pipe to read log messages from
 * and sets up read_set for select(). Then it runs main control loop, using select() to make actions event-driven.
 * Arguments:
 *    - void *args: (always NULL)
 * Return:
 *    - NULL
 */
static void* process_tcp (void* tcp_args)
{
    tcp_conn_args_t* args = (tcp_conn_args_t*) tcp_args;
	
	//Open FIFO pipe for logs
	if ((log_fd = open(FIFO_NAME, O_RDWR | O_NONBLOCK)) == -1) {
		perror("fifo open");
        log_printf(ERROR, " could not open log FIFO on %d", args->client);
	}
	log_printf(DEBUG, "Opened up log FIFO %d", log_fd);
    // open challenge_fd for reading here
	log_printf(DEBUG, "conn_fd inside process %d", args->conn_fd);
	//variables used for waiting for something to do using select()
	int maxfdp1 = ((args->conn_fd > log_fd) ? args->conn_fd : log_fd) + 1;
	fd_set read_set;
	//main control loop that is responsible for sending and receiving data
	while (1) {
		FD_ZERO(&read_set);
		log_printf(DEBUG, "max set size %d, conn_fd %d", FD_SETSIZE, args->conn_fd);
		FD_SET(args->conn_fd, &read_set);
		log_printf(DEBUG, "Setting fd_set");
		FD_SET(log_fd, &read_set);
		log_printf(DEBUG, "reading from aux SHM");
        if (robot_desc_read(RUN_MODE) == CHALLENGE) {
			FD_SET(challenge_fd, &read_set);
		}
		log_printf(DEBUG, "Setting cleanup handler");
		//prepare to accept cancellation requests over the select
		pthread_cleanup_push(tcp_conn_cleanup, args);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_testcancel();
		
		//wait for something to happen
		log_printf(DEBUG, "waiting on select");
		if (select(maxfdp1, &read_set, NULL, NULL, NULL) < 0) {
			perror("select");
            log_printf(ERROR, "Failed to wait for select in control loop for client %d", args->client);
		}
		
		//deny all cancellation requests until the next loop
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		pthread_cleanup_pop(0); //definitely do NOT execute the cleanup handler!
		
		//send a new log message if one is available
		if (FD_ISSET(log_fd, &read_set)) {
			log_printf(DEBUG, "received message on log");
			send_log_msg(args->conn_fd, log_fd);
		}
		//receive new message on socket if it is ready
		if (FD_ISSET(args->conn_fd, &read_set)) {
			log_printf(DEBUG, "received message on socket");
			int err = recv_new_msg(args->conn_fd, challenge_fd); 
			if (err == -1) { 
				log_printf(WARN, "client %d has disconnected", args->client);
				break;
			}
		}
	}
	
	//call the cleanup function
	tcp_conn_cleanup(tcp_args);
	return NULL;
}

/*
 * Start the main TCP connection control thread. Does not block.
 * Should be called when the client has requested a connection to Runtime.
 * Arguments:
 *    - int connfd: connection socket descriptor on which there is the established connection with the client
 */
void start_tcp_conn (robot_desc_val_t client, int conn_fd, int send_logs)
{
	tcp_conn_args_t* args = malloc(sizeof(tcp_conn_args_t));
	args->conn_fd = conn_fd;
	args->client = client;
	args->send_logs = send_logs;
    pthread_t* tid;
	if (client == DAWN) {
        tid = &dawn_tid;
    }
    else if (client == SHEPHERD) {
        tid = &shepherd_tid;
    }
    else {
        log_printf(ERROR, "Invalid TCP client %d", client);
    }
	log_printf(DEBUG, "conn_fd %d", conn_fd);
	//create the main control thread
	if (pthread_create(tid, NULL, process_tcp, args) != 0) {
		perror("pthread_create");
        log_printf(ERROR, "Failed to create main TCP thread for %d", client);
		return;
	}
	robot_desc_write(client, CONNECTED);
	log_printf(DEBUG, "successfully made connection with %d and started thread!", client);
}


/*
 * Stops the dawn connection control thread cleanly. May block briefly to allow
 * main control thread to finish what it's doing. Should be called right before the net_handler main loop terminates.
 */
void stop_tcp_conn (robot_desc_val_t client)
{
    pthread_t tid;
    if (client == DAWN) {
        tid = dawn_tid;
    }
    else if (client == SHEPHERD) {
        tid = shepherd_tid;
    }
    else {
        log_printf(ERROR, "Invalid TCP client %d", client);
    }

	if (pthread_cancel(tid) != 0) {
		perror("pthread_cancel");
        log_printf(ERROR, "Failed to cancel TCP client thread for %d", client);
	}
	if (pthread_join(tid, NULL) != 0) {
		perror("pthread_join");
        log_printf(ERROR, "Failed to join on TCP client thread for %d", client);
	}
	robot_desc_write(client, DISCONNECTED);
	log_printf(DEBUG, "disconnected with client %d and stopped thread", client);
}
