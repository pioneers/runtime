#include "shepherd_conn.h"

//used for cleaning up shepherd thread on cancel
typedef struct cleanup_objs {
	int connfd;
	int challenge_fd;
	Text *challenge_data_msg;
} cleanup_objs_t;

int shep_connfd;

pthread_t shep_tid;

/*
 * Clean up memory and file descriptors before exiting from shepherd connection main control loop
 * Arguments:
 *    - void *args: should be a pointer to cleanup_objs_t populated with the listed descriptors and pointers
 */
static void shep_conn_cleanup (void *args)
{
	cleanup_objs_t *cleanup_objs = (cleanup_objs_t *) args;
	
	free(cleanup_objs->challenge_data_msg->payload);
	
	if (close(cleanup_objs->connfd) != 0) {
		error("close: connfd @shep");
	}
	if (close(cleanup_objs->challenge_fd) != 0) {
		error("close: challenge_fd @shep");
	}
	robot_desc_write(SHEPHERD, DISCONNECTED);
}

/* 
 * Send a challenge data message on the TCP connection to Shepherd.
 * Arguments:
 *    - int *connfd: pointer to connection socket descriptor on which to write to the TCP port
 *    - int results_fd: file descriptor of FIFO pipe from which to read challenge results from executor
 *    - Text *challenge_data_msg: pointer to Text protobuf message structure in which to build the challenge data message
 */
static void send_challenge_msg (int *connfd, int results_fd, Text *challenge_data_msg)
{
	const int max_len = 1000;   //max len of a challenge data message
	uint8_t *send_buf;          //buffer to send data on
	unsigned len_pb;            //length of serialized protobuf
	uint16_t len_pb_uint16;     //length of serialized protobuf, as uint16_t
	size_t result_len;          //length of results string
	char buf[max_len];          //buffer that results are read into, from which it is copied into text message
	
	//keep trying to read the line until you get it; copy that line into challenge_data_msg.payload[0]
	//TODO: change this to deal with FIFO pipe
	//while (read(buf, max_len, results_fd) != 0);
	result_len = strlen((const char *) buf);
	challenge_data_msg->payload[0] = (char *) malloc(sizeof(char) * result_len);
	strcpy(challenge_data_msg->payload[0], (const char *) buf);
	
	//prepare the message for sending
	len_pb = text__get_packed_size(challenge_data_msg);
	send_buf = prep_buf(CHALLENGE_DATA_MSG, len_pb, &len_pb_uint16);
	text__pack(challenge_data_msg, (void *) (send_buf + 3));  //pack message into the rest of send_buf (starting at send_buf[3] onward)
	
	//send message on socket
	if (writen(*connfd, send_buf, (size_t) (len_pb_uint16 + 3)) == -1) {
		error("write: sending challenge data message failed @shep");
	}
	
	//free all allocated memory
	free(challenge_data_msg->payload[0]);
	free(send_buf);
}

/*
 * Receives new message from Shepherd on TCP connection and processes the message.
 * Arguments:
 *    - int *connfd: pointer to connection socket descriptor from which to read the message
 *    - int results_fd: file descriptor of FIFO pipe to executor to which to write challenge input data if received
 *    - Text *challenge_data_msg: pointer to Text protobuf message to use for processing Challenge Data messages
 *    - RunMode *run_mode_msg: pointer to RunMode protobuf message to use for processing RunMode messages
 *    - StartPos *startpos_msg: pointer to StartPos protobuf message to use for processing StartPos messages
 *    - int *retval: pointer to integer in which return status will be stored
 *            - 0 if message received and processed
 *            - -1 if message could not be parsed because Shepherd disconnected and connection closed
 */
static void recv_new_msg (int *connfd, int results_fd, Text *challenge_data_msg, RunMode *run_mode_msg, StartPos *startpos_msg, int *retval)
{
	net_msg_t msg_type;           //message type
	uint16_t len_pb;              //length of incoming serialized protobuf message
	uint8_t buf[MAX_SIZE_BYTES];  //buffer to read raw data into
	size_t challenge_len;         //length of the challenge data received string
	
	//get the msg type and length in bytes; read the rest of msg into buf according to len_pb
	//if parse_msg doesn't return 0, we found EOF and shepherd disconnected
	if (parse_msg(connfd, &msg_type, &len_pb, buf) != 0) {
		*retval = -1;
		return;
	};
	
	//unpack according to message
	if (msg_type == CHALLENGE_DATA_MSG) {
		challenge_data_msg = text__unpack(NULL, len_pb, buf);
		
		//TODO: write the provided input data for the challenge to FIFO pipe to executor
		challenge_len = strlen(challenge_data_msg->payload[0]);
		if (writen(results_fd, challenge_data_msg->payload[0], challenge_len) == -1) {
			error("fwrite: could not write to results.txt for challenge data @shep");
		}
		
		text__free_unpacked(challenge_data_msg, NULL);
	} else if (msg_type == RUN_MODE_MSG) {
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
	} else if (msg_type == STARTPOS) {
		startpos_msg = start_pos__unpack(NULL, len_pb, buf);
		
		//write the specified start pos to the STARTPOS field of the robot description
		switch (startpos_msg->pos) {
			case (POS__LEFT):
				log_runtime(DEBUG, "robot is in LEFT start position");
				robot_desc_write(STARTPOS, LEFT);
				break;
			case (POS__RIGHT):
				log_runtime(DEBUG, "robot is in RIGHT start position");
				robot_desc_write(STARTPOS, RIGHT);
				break;
			default:
				log_runtime(WARN, "entered unknown start position");
		}
		start_pos__free_unpacked(startpos_msg, NULL);
	} else {
		log_printf(WARN, "unknown message type %d; shepherd should only send RUN_MODE (2), STARTPOS (3), or CHALLENGE_DATA (4)", msg_type);
	}
	*retval = 0;
}

/*
 * Main control loop for shepherd connection. Sets up connection by opening up pipe to read/write challenge data messages
 * and sets up read_set for select(). Then it runs main control loop, using select() to make actions event-driven.
 * Arguments:
 *    - void *args: (always NULL)
 * Return:
 *    - NULL
 */
static void *shepherd_conn (void *args)
{
	int *connfd = &shep_connfd;
	int results_fd = 0;
	int recv_success = 0; //used to tell if message was received successfully
	
	//initialize the protobuf messages
	Text challenge_data_msg = TEXT__INIT;
	challenge_data_msg.payload = (char **) malloc(sizeof(char *) * 1);
	challenge_data_msg.n_payload = 1;
	RunMode run_mode_msg = RUN_MODE__INIT;
	StartPos startpos_msg = START_POS__INIT;
	
	//TODO: Open connection to FIFO pipe in /tmp/* to executor for sending/receiving challenge results
	/*
	if ((results_fp = fopen("../executor/results.txt", "r+b")) == NULL) { //TODO: NOT IMPLEMENTED YET
		error("fopen: could not open results file for reading @shep");
		return NULL;
	}
	if (fseek(results_fp, 0, SEEK_END) != 0) { //TODO: NOT IMPLEMENTED YET
		error("fssek: could not seek to end of results file @shep");
		return NULL;
	}
	*/
	
	//variables used for waiting for something to do using select()
	int maxfdp1 = ((*connfd > results_fd) ? *connfd : results_fd) + 1;
	fd_set read_set;
	FD_ZERO(&read_set);
	
	//set cleanup objects for cancellation handler
	cleanup_objs_t cleanup_objs = { *connfd, results_fd, &challenge_data_msg };
	
	//main control loop that is responsible for sending and receiving data
	while (1) {
		FD_SET(*connfd, &read_set);
		if (robot_desc_read(RUN_MODE) == CHALLENGE) {
			FD_SET(results_fd, &read_set);
		}
		
		//prepare to accept cancellation requests over the select
		pthread_cleanup_push(shep_conn_cleanup, (void *) &cleanup_objs);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_testcancel();
		
		//wait for something to happen
		if (select(maxfdp1, &read_set, NULL, NULL, NULL) < 0) {
			error("select: @shep");
		}
		
		//deny all cancellation requests until the next loop
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		pthread_cleanup_pop(0); //definitely do NOT execute the cleanup handler!
		
		//send the challenge results if it is ready
		if (FD_ISSET(results_fd, &read_set)) {
			send_challenge_msg(connfd, results_fd, &challenge_data_msg);
		}
		//receive new message on socket if it is ready
		if (FD_ISSET(*connfd, &read_set)) {
			recv_new_msg(connfd, results_fd, &challenge_data_msg, &run_mode_msg, &startpos_msg, &recv_success); 
			if (recv_success == -1) { //shepherd disconnected
				log_runtime(WARN, "thread detected shepherd disocnnected @shep");
				break;
			}
		}
	}
	
	//call the cleanup function
	shep_conn_cleanup((void *) &cleanup_objs);
	return NULL;
}

/*
 * Start the main shepherd connection control thread. Does not block.
 * Should be called when Shepherd has requested a connection to Runtime.
 * Arguments:
 *    - int connfd: connection socket descriptor on which there is the established connection with Shepherd
 */
void start_shepherd_conn (int connfd)
{
	shep_connfd = connfd;
	
	//create thread, give it connfd, write that shepherd is connected
	if (pthread_create(&shep_tid, NULL, shepherd_conn, NULL) != 0) {
		error("pthread_create: @shep");
		return;
	}
	//disable cancellation requests on the thread until main loop begins
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	robot_desc_write(SHEPHERD, CONNECTED);
	log_runtime(INFO, "successfully made connection with Shepherd and started thread!");
}

/*
 * Stops the shepherd connection control thread cleanly. May block briefly to allow
 * main control thread to finish what it's doing. Should be called right before the net_handler main loop terminates.
 */
void stop_shepherd_conn ()
{
	if (pthread_cancel(shep_tid) != 0) {
		error("pthread_cancel: @shep");
	}
	if (pthread_join(shep_tid, NULL) != 0) {
		error("pthread_join: @shep");
	}
	robot_desc_write(SHEPHERD, DISCONNECTED);
	log_runtime(INFO, "disconnected with Shepherd and stopped thread");
}