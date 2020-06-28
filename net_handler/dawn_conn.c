#include "shepherd_conn.h"

//used for cleaning up shepherd thread on cancel
typedef struct cleanup_objs {
	int connfd;
	FILE *log_fp;
	Text *log_msg;
} cleanup_objs_t;

pthread_t dawn_tid;

//cleanup function for dawn_conn thread
static void dawn_conn_cleanup (void *args)
{
	cleanup_objs_t *cleanup_objs = (cleanup_objs_t *) args;
	
	free(cleanup_objs->log_msg->payload);
	
	fclose(cleanup_objs->log_fp);
	
	if (close(cleanup_objs->connfd) != 0) {
		error("close: connfd @shep");
	}
	robot_desc_write(DAWN, DISCONNECTED);
}

//TODO: try and figure out the most optimized way to set up the log message (maybe write a custom allocator?)
static void send_log_msg (int *connfd, FILE *log_fp, Text *log_msg)
{
	uint8_t *send_buf;        //buffer to send data on
	unsigned len_pb;          //length of serialized protobuf
	size_t next_line_len;     //length of the next log line
	char buf[MAX_LOG_LEN];    //buffer that next log line is read into, from which it is copied into text message
	
	//read in log lines until there are no more to read
	log_msg->n_payload = 0;
	for (log_msg->n_payload = 0; (fgets(buf, LOG_MSG_MAXLEN, log_fp) != NULL) && (log_msg->n_payload < MAX_NUM_LOGS); log_msg->n_payload++) {
		next_line_len = strlen((const char *) buf);
		log_msg->payload[log_msg->n_payload] = (char *) malloc(sizeof(char) * next_line_len);
		strcpy(log_msg->payload[log_msg->n_payload], (const char *) buf);
	}
	
	//prepare the message for sending
	len_pb = text__get_packed_size(log_msg);
	prep_buf(send_buf, LOG_MSG, len_pb);
	text__pack(log_msg, (void *) (send_buf + 3));  //pack message into the rest of send_buf (starting at send_buf[3] onward)
	
	//send message on socket
	if (writen(*connfd, send_buf, (size_t) (len_pb + 3)) == -1) {
		error("write: sending log message failed @shep");
	}
	
	//free all allocated memory
	for (int i = 0; i < log_msg->n_payload; i++) {
		free(log_msg->payload[i]);
	}
	free(send_buf);
}

static void recv_new_msg (int *connfd, FILE *results_fp, Text *challenge_data_msg, RunMode *run_mode_msg, int *retval)
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
		
		//write the provided input data for the challenge to results.txt (results_fp)
		challenge_len = strlen(challenge_data_msg->payload[0]);
		if (fwrite(challenge_data_msg->payload[0], sizeof(char), challenge_len, results_fp) <= 0) {
			error("fwrite: could not write to results.txt for challenge data @shep");
		}
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
	} else {
		log_printf(WARN, "unknown message type %d; shepherd should only send RUN_MODE (2) or CHALLENGE_DATA (3)", msg_type);
	}
	*retval = 0;
}

static void *dawn_conn (void *args)
{
	int *connfd = (int *) args;
	int recv_success = 0; //used to tell if message was received successfully
	
	//initialize the protobuf messages
	Text log_msg = TEXT__INIT;
	log_msg.payload = (char **) malloc(sizeof(char *) * MAX_NUM_LOGS);
	
	DevData dev_data_msg = DEV_DATA__INIT;
	//something something something
	
	RunMode run_mode_msg = RUN_MODE__INIT;
	
	//Open up log file and challenge results file; seek to the end of each file
	FILE *log_fp;
	if ((log_fp = fopen("../logger/runtime_log.log", "r")) == NULL) {
		error("fopen: could not open log file for reading @shep");
		return NULL;
	}
	if (fseek(log_fp, 0, SEEK_END) != 0) {
		error("fseek: could not seek to end of log file @shep");
		return NULL;
	}
	
	//variables used for waiting for something to do using select()
	int maxfdp1 = (fileno(log_fp) > *connfd) ? fileno(log_fp) + 1 : (*connfd) + 1;
	fd_set read_set;
	FD_ZERO(&read_set);
	
	//set cleanup objects for cancellation handler
	cleanup_objs_t cleanup_objs = { *connfd, log_fp, &log_msg };
	
	//main control loop that is responsible for sending and receiving data
	//TODO: maybe try using pselect instead of select
	while (1) {
		FD_SET(fileno(log_fp), &read_set);
		FD_SET(*connfd, &read_set);
		
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
		if (FD_ISSET(fileno(log_fp), &read_set)) {
			send_log_msg(connfd, log_fp, &log_msg);
		}
		//receive new message on socket if it is ready
		if (FD_ISSET(*connfd, &read_set)) {
			recv_new_msg(connfd, results_fp, &dev_data_msg, &run_mode_msg, &recv_success); 
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

void start_dawn_conn (int connfd)
{
	//create thread, give it connfd, write that shepherd is connected
	if (pthread_create(&dawn_tid, NULL, dawn_conn, &connfd) != 0) {
		error("pthread_create: @dawn");
		return;
	}
	//disable cancellation requests on the thread until main loop begins
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	robot_desc_write(DAWN, CONNECTED);
	log_runtime(INFO, "successfully made connection with Dawn and started thread!");
}

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