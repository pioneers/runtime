#include "dawn_conn.h"

//used for cleaning up dawn thread on cancel
typedef struct cleanup_objs {
	int connfd;
	int fifofd;
	Text *log_msg;
} cleanup_objs_t;

int dawn_connfd;

pthread_t dawn_tid;

//for the internal log message buffering
char glob_buf[MAX_LOG_LEN];
int glob_buf_ix = 0;  //index of glob_buf we are currently operating on
int glob_buf_end = 0; //index of first invalid char in glob_buf

/*
 * Clean up memory and file descriptors before exiting from dawn connection main control loop
 * Arguments:
 *    - void *args: should be a pointer to cleanup_objs_t populated with the listed descriptors and pointers
 */
static void dawn_conn_cleanup (void *args)
{
	cleanup_objs_t *cleanup_objs = (cleanup_objs_t *) args;
	
	free(cleanup_objs->log_msg->payload);
	
	if (close(cleanup_objs->fifofd) != 0) {
		error("close: fifofd @dawn");
	}
	
	if (close(cleanup_objs->connfd) != 0) {
		error("close: connfd @dawn");
	}
	robot_desc_write(DAWN, DISCONNECTED);
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
	
	//if we got to the end of glob_buf, we need to read more from pipe
	if (glob_buf_ix == glob_buf_end) {
		n_read = read(fifofd, glob_buf, MAX_LOG_LEN);
		//handle error cases and return NULL
		if (n_read == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) { //if no more data on fifo pipe
				*retval = 1;
			} else { //some other error
				*retval = -1;
				error("read: log fifo @dawn");
			}
			return NULL;
		} else if (n_read == 0) { //EOF
			*retval = 0;
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
 * Send a log message on the TCP connection to Dawn. Reads lines from the pipe until there is no more data
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
static int send_log_msg (int *connfd, int fifofd, Text *log_msg)
{	
	char *nextline;            //next log line read from FIFO pipe
	int retval;                //return value for readline()
	unsigned len_pb;           //length of serialized protobuf message
	uint16_t len_pb_uint16;    //length of serialized protobuf message, as uin16_t
	uint8_t *send_buf;         //buffer for constructing the final log message
	
	//read in log lines until there are no more to read, or we read MAX_NUM_LOGS lines
	log_msg->n_payload = 0;
	while (log_msg->n_payload < MAX_NUM_LOGS) {
		if ((nextline = readline(fifofd, &retval)) == NULL) {
			if (retval == 1) { //if no more data to read (EWOULDBLOCK or EAGAIN)
				break;
			} else if (retval == 0) {  //if EOF (all writers of FIFO pipe crashed; shouldn't happen)
				return 0;
			} else if  (retval == -1) {  //some other error
				return -1;
			}
		}
		log_msg->payload[log_msg->n_payload] = nextline;
		log_msg->n_payload++;
	}
	
	//prepare the message for sending
	len_pb = text__get_packed_size(log_msg);
	send_buf = prep_buf(LOG_MSG, len_pb, &len_pb_uint16);
	text__pack(log_msg, (void *) (send_buf + 3));  //pack message into the rest of send_buf (starting at send_buf[3] onward)
	
	//send message on socket
	if (writen(*connfd, send_buf, (size_t) (len_pb_uint16 + 3)) == -1) {
		error("write: sending log message failed @dawn");
	}
	
	//free all allocated memory
	for (int i = 0; i < log_msg->n_payload; i++) {
		free(log_msg->payload[i]);
	}
	free(send_buf);
	return 1;
}

/*
 * Receives new message from Dawn on TCP connection and processes the message.
 * Arguments:
 *    - int *connfd: pointer to connection socket descriptor from which to read the message
 *    - RunMode *run_mode_msg: pointer to RunMode protobuf message to use for processing RunMode messages
 *    - DevData *dev_data_msg: pointer to DevData protobuf message to use for processing DevData messages
 *    - int *retval: pointer to integer in which return status will be stored
 *            - 0 if message received and processed
 *            - -1 if message could not be parsed because Dawn disconnected and connection closed
 */
static void recv_new_msg (int *connfd, RunMode *run_mode_msg, DevData *dev_data_msg, int *retval)
{
	net_msg_t msg_type;           //message type
	uint16_t len_pb;              //length of incoming serialized protobuf message
	uint8_t buf[MAX_SIZE_BYTES];  //buffer to read raw data into
	
	//get the msg type and length in bytes; read the rest of msg into buf according to len_pb
	//if parse_msg doesn't return 0, we found EOF and dawn disconnected
	if (parse_msg(connfd, &msg_type, &len_pb, buf) != 0) {
		*retval = -1;
		return;
	};
	
	//unpack according to message
	if (msg_type == RUN_MODE_MSG) {
		run_mode_msg = run_mode__unpack(NULL, len_pb, buf);
		
		//write the specified run mode to the RUN_MODE field of the robot description
		switch (run_mode_msg->mode) {
			case (MODE__IDLE):
				log_runtime(DEBUG, "entering IDLE mode");
				robot_desc_write(RUN_MODE, IDLE);
				break;
			case (MODE__AUTO):
				log_runtime(DEBUG, "entering AUTO mode");
				robot_desc_write(RUN_MODE, AUTO);
				break;
			case (MODE__TELEOP):
				log_runtime(DEBUG, "entering TELEOP mode");
				robot_desc_write(RUN_MODE, TELEOP);
				break;
			case (MODE__ESTOP):
				log_runtime(DEBUG, "ESTOP RECEIVED! entering IDLE mode");
				robot_desc_write(RUN_MODE, IDLE);
				break;
			case (MODE__CHALLENGE): 
				log_runtime(DEBUG, "entering CHALLENGE mode; running coding challenges!");
				robot_desc_write(RUN_MODE, CHALLENGE);
				break;
			default:
				log_runtime(WARN, "requested robot to enter unknown robot mode");
		}
		run_mode__free_unpacked(run_mode_msg, NULL);
	} else if (msg_type == DEVICE_DATA_MSG) {
		dev_data_msg = dev_data__unpack(NULL, len_pb, buf);
		
		//TODO: do something with the data (give it to UDP suite to handle)
		
		dev_data__free_unpacked(dev_data_msg, NULL);
	} else {
		log_printf(WARN, "unknown message type %d; dawn should only send RUN_MODE (2) or CHALLENGE_DATA (3)", msg_type);
	}
	*retval = 0;
}

/*
 * Main control loop for dawn connection. Sets up connection by opening up pipe to read log messages from
 * and sets up read_set for select(). Then it runs main control loop, using select() to make actions event-driven.
 * Arguments:
 *    - void *args: (always NULL)
 * Return:
 *    - NULL
 */
static void *dawn_conn (void *args)
{
	int *connfd = &dawn_connfd;
	int fifofd = -1;      //fd of the open FIFO pipe
	int recv_success = 0; //used to tell if message was received successfully
	
	//initialize the protobuf messages
	Text log_msg = TEXT__INIT;
	log_msg.payload = (char **) malloc(sizeof(char *) * MAX_NUM_LOGS);
	RunMode run_mode_msg = RUN_MODE__INIT;
	DevData dev_data_msg = DEV_DATA__INIT;
	
	//Open FIFO pipe for logs
	if ((fifofd = open(FIFO_NAME, O_RDWR | O_NONBLOCK)) == -1) {
		perror("open: could not open log FIFO @dawn");
	}
	
	//variables used for waiting for something to do using select()
	int maxfdp1 = ((*connfd > fifofd) ? *connfd : fifofd) + 1;
	fd_set read_set;
	FD_ZERO(&read_set);
	
	//set cleanup objects for cancellation handler
	cleanup_objs_t cleanup_objs = { *connfd, fifofd, &log_msg, };
	
	//main control loop that is responsible for sending and receiving data
	while (1) {
		FD_SET(*connfd, &read_set);
		FD_SET(fifofd, &read_set);
		
		//prepare to accept cancellation requests over the select
		pthread_cleanup_push(dawn_conn_cleanup, (void *) &cleanup_objs);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_testcancel();
		
		//wait for something to happen
		if (select(maxfdp1, &read_set, NULL, NULL, NULL) < 0) {
			error("select: @dawn");
		}
		
		//deny all cancellation requests until the next loop
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		pthread_cleanup_pop(0); //definitely do NOT execute the cleanup handler!
		
		//send a new log message if one is available
		if (FD_ISSET(fifofd, &read_set)) {
			send_log_msg(connfd, fifofd, &log_msg);
		}
		//receive new message on socket if it is ready
		if (FD_ISSET(*connfd, &read_set)) {
			recv_new_msg(connfd, &run_mode_msg, &dev_data_msg, &recv_success); 
			if (recv_success == -1) { //dawn disconnected
				log_runtime(WARN, "thread detected dawn disocnnected @dawn");
				break;
			}
		}
	}
	
	//call the cleanup function
	dawn_conn_cleanup((void *) &cleanup_objs);
	return NULL;
}

/*
 * Start the main dawn connection control thread. Does not block.
 * Should be called when Dawn has requested a connection to Runtime.
 * Arguments:
 *    - int connfd: connection socket descriptor on which there is the established connection with Dawn
 */
void start_dawn_conn (int connfd)
{
	dawn_connfd = connfd;
	
	//create the main control thread
	if (pthread_create(&dawn_tid, NULL, dawn_conn, NULL) != 0) {
		error("pthread_create: @dawn");
		return;
	}
	//disable cancellation requests on the thread until main loop begins
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	robot_desc_write(DAWN, CONNECTED);
	log_runtime(INFO, "successfully made connection with Dawn and started thread!");
}

/*
 * Stops the dawn connection control thread cleanly. May block briefly to allow
 * main control thread to finish what it's doing. Should be called right before the net_handler main loop terminates.
 */
void stop_dawn_conn ()
{
	if (pthread_cancel(dawn_tid) != 0) {
		error("pthread_cancel: @dawn");
	}
	if (pthread_join(dawn_tid, NULL) != 0) {
		error("pthread_join: @dawn");
	}
	robot_desc_write(DAWN, DISCONNECTED);
	log_runtime(INFO, "disconnected with Dawn and stopped thread");
}
