#include "shepherd_conn.h"

//used for cleaning up shepherd thread on cancel
typedef struct cleanup_objs {
	int connfd;
	Text *challenge_data_msg;
} cleanup_objs_t;

int shep_connfd;

pthread_t shep_tid;

//cleanup function for shepherd_conn thread
static void shep_conn_cleanup (void *args)
{
	cleanup_objs_t *cleanup_objs = (cleanup_objs_t *) args;
	
	free(cleanup_objs->challenge_data_msg->payload);
	
	if (close(cleanup_objs->connfd) != 0) {
		error("close: connfd @shep");
	}
	robot_desc_write(SHEPHERD, DISCONNECTED);
}

//TODO: try and figure out the most optimized way to set up the log message (maybe write a custom allocator?)
static void send_log_msg (int *connfd, FILE *log_fp, Text *log_msg)
{
	uint8_t *send_buf;        //buffer to send data on
	unsigned len_pb;          //length of serialized protobuf
	uint16_t len_pb_uint16;   //length of serialized protobuf, as uint16_t
	size_t next_line_len;     //length of the next log line
	char buf[MAX_LOG_LEN];    //buffer that next log line is read into, from which it is copied into text message
	
	//read in log lines until there are no more to read
	for (log_msg->n_payload = 0; (fgets(buf, MAX_LOG_LEN, log_fp) != NULL) && (log_msg->n_payload < MAX_NUM_LOGS); log_msg->n_payload++) {
		next_line_len = strlen((const char *) buf);
		log_msg->payload[log_msg->n_payload] = (char *) malloc(sizeof(char) * next_line_len);
		strcpy(log_msg->payload[log_msg->n_payload], (const char *) buf);
	}
	
	if (ferror(log_fp) != 0) {
		printf("error occured when reading log file\n");
	}
	
	//prepare the message for sending
	len_pb = text__get_packed_size(log_msg);
	send_buf = prep_buf(LOG_MSG, len_pb, &len_pb_uint16);
	if (len_pb_uint16 == 0) {
		printf("no data\n");
		free(send_buf);
		return;
	}
	text__pack(log_msg, (void *) (send_buf + 3));  //pack message into the rest of send_buf (starting at send_buf[3] onward)
	
	printf("send_buf = %hhu %hhu %hhu\n", send_buf[0], send_buf[1], send_buf[2]);
	
	//send message on socket
	if (writen(*connfd, send_buf, (size_t) (len_pb_uint16 + 3)) == -1) {
		error("write: sending log message failed @shep");
	}
	
	//free all allocated memory
	for (int i = 0; i < log_msg->n_payload; i++) {
		free(log_msg->payload[i]);
	}
	free(send_buf);
}

static void send_challenge_msg (int *connfd, int results_fd, Text *challenge_data_msg)
{
	const int max_len = 1000;   //max len of a challenge data message
	uint8_t *send_buf;          //buffer to send data on
	unsigned len_pb;            //length of serialized protobuf
	uint16_t len_pb_uint16;     //length of serialized protobuf, as uint16_t
	size_t result_len;          //length of results string
	char buf[max_len];          //buffer that results are read into, from which it is copied into text message
	
	//keep trying to read the line until you get it; copy that line into challenge_data_msg.payload[0]
	//TODO: change this to deal with some sort of stream instead of a file
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

static void recv_new_msg (int *connfd, int results_fd, Text *challenge_data_msg, RunMode *run_mode_msg, StartPos *startpos_msg, int *retval)
{
	net_msg_t msg_type;           //message type
	uint16_t len_pb;              //length of incoming serialized protobuf message
	uint8_t buf[MAX_SIZE_BYTES];  //buffer to read raw data into
	int challenge_len;            //length of the challenge data received string
	
	//get the msg type and length in bytes; read the rest of msg into buf according to len_pb
	//if parse_msg doesn't return 0, we found EOF and shepherd disconnected
	if (parse_msg(connfd, &msg_type, &len_pb, buf) != 0) {
		*retval = -1;
		return;
	};
	
	//unpack according to message
	if (msg_type == CHALLENGE_DATA_MSG) {
		challenge_data_msg = text__unpack(NULL, len_pb, buf);
		
		//TODO: write the provided input data for the challenge to wherever it needs to go
		/*
		challenge_len = strlen(challenge_data_msg->payload[0]);
		if (fwrite(challenge_data_msg->payload[0], sizeof(char), challenge_len, results_fp) <= 0) {
			error("fwrite: could not write to results.txt for challenge data @shep");
		}
		*/
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
	
	//TODO: Open connection to method of getting challenge data results
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
	cleanup_objs_t cleanup_objs = { *connfd, &challenge_data_msg };
	
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