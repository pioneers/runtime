#include "shepherd_conn.h"

//used for cleaning up shepherd thread on cancel
typedef struct cleanup_objs {
	int connfd;
	FILE *log_fp;
	FILE *results_fp;
	Text *log_msg;
	Text *challenge_data_msg;
} cleanup_objs_t;

pthread_t shep_tid;

static void error (char *msg)
{
	perror(msg);
	log_runtime(ERROR, msg); //send the message to the logger
}

//read n bytes from fd into buf; return number of bytes read into buf
static ssize_t readn (int fd, void *buf, size_t n)
{
	size_t n_remain = n;
	ssize_t n_read;
	char *curr = buf;
	
	while (n_remain > 0) {
		if ((n_read = read(fd, buf, n_remain)) < 0) {
			if (errno == EINTR) { //read interrupted by signal; read again
				n_read = 0;
			} else {
				error("read: reading from connfd failed @shep");
				return -1;
			}
		} else if (n_read == 0) { //received EOF
			break;
		}
		n_remain -= n_read;
		curr += n_read;
	}
	return (n - n_remain);
}

//write n bytes from buf to fd; return number of bytes written to fd
static ssize_t writen (int fd, const void *buf, size_t n)
{
	size_t n_remain = n;
	ssize_t n_written;
	const char *ptr = buf;
	
	while (n_remain > 0) {
		if ((n_written = write(fd, buf, n_remain)) <= 0) {
			if (n_written < 0 && errno == EINTR) { //write interrupted by signal, write again
				n_written = 0;
			} else {
				error("write: writing to connfd failed @shep");
				return -1;
			}
		}
		n_remain -= n_written;
		ptr += n_written;
	}
	return n;
}

//prepares send_buf by prepending msg_type into first byte, and len_pb into second and third bytes
//caller must free send_buf
static void prep_buf (uint8_t *send_buf, net_msg_t msg_type, unsigned len_pb)
{
	uint16_t uint16_ptr; //used to insert a uint16_t into an array of uint8_t
	
	send_buf = (uint8_t *) malloc(sizeof(uint8_t) * (len_pb + 3)); // +3 because 1 byte for message type, 2 bytes for message length
	send_buf[0] = msg_type;             //write the message type to the first byte of send_buf    
	uint16_ptr = send_buf[1];           //make uint16_ptr point to send_buf[1], where we will insert len_pb
	uint16_ptr[0] = (uint16_t) len_pb;  //insert len_pb cast to uint16_t into send_buf[1] and send_buf[2]
}

//cleanup function for shepherd_conn thread
void *shep_conn_cleanup (void *args)
{
	cleanup_objs_t *cleanup_objs = (cleanup_objs_t *) args;
	
	free(cleanup_objs->log_msg->payload);
	free(cleanup_objs->challenge_data_msg->payload);
	
	fclose(cleanup_objs->log_fp);
	fclose(cleanup_objs->results_fp);
	
	if (close(cleanup_objs->connfd) != 0) {
		error("close: connfd @shep");
	}
	robot_desc_write(SHEPHERD, DISCONNECTED);
	
	return NULL;
}

//TODO: try and figure out the most optimized way to set up the log message (maybe write a custom allocator?)
void send_log_msg (int *connfd, FILE *log_fp, Text *log_msg)
{
	uint8_t *send_buf;          //buffer to send data on
	unsigned len_pb;            //length of serialized protobuf
	size_t next_line_len;       //length of the next log line
	char buf[LOG_MSG_MAXLEN];   //buffer that next log line is read into, from which it is copied into text message
	ssize_t n_sent;             //number of bytes sent on TCP port 
	
	//read in log lines until there are no more to read
	log_msg.n_payload = 0;
	for (log_msg.n_payload = 0; (fgets(buf, LOG_MSG_MAXLEN, log_fp) != NULL) && (log_msg.n_payload < MAX_NUM_LOGS); log_msg.n_payload++) {
		next_line_len = strlen((const char *) buf);
		log_msg.payload[log_msg.n_payload] = (char *) malloc(sizeof(char) * next_line_len);
		strcpy(log_msg.payload[log_msg.n_payload], (const char *) buf);
	}
	
	//prepare the message for sending
	len_pb = text__get_packed_size(log_msg);
	prep_buf(send_buf, LOG, len_pb);
	text__pack(&log_msg, (void *) (send_buf + 3));  //pack message into the rest of send_buf (starting at send_buf[3] onward)
	
	//send message on socket
	if (writen(*connfd, send_buf, (size_t) (len_pb + 3)) == -1) {
		error("write: sending log message failed @shep");
	}
	
	//free all allocated memory
	for (int i = 0; i < log_msg.n_payload; i++) {
		free(log_msg.payload[i]);
	}
	free(send_buf);
}

void send_challenge_msg (int *connfd, FILE *results_fp, Text *challenge_data_msg)
{
	uint8_t *send_buf;          //buffer to send data on
	unsigned len_pb;            //length of serialized protobuf
	size_t result_len;          //length of results string
	char buf[LOG_MSG_MAXLEN];   //buffer that next log line is read into, from which it is copied into text message
	
	//keep trying to read the line until you get it; copy that line into challenge_data_msg.payload[0]
	while (fgets(buf, LOG_MSG_MAXLEN, results_fp) != NULL);
	result_len = strlen((const char *) buf);
	challenge_data_msg.payload[0] = (char *) malloc(sizeof(char) * result_len);
	strcpy(challenge_data_msg.payload[0], (const char *) buf);
	
	//prepare the message for sending
	len_pb = text__get_packed_size(challenge_data_msg);
	prep_buf(send_buf, CHALLENGE_DATA, len_pb);
	text__pack(&challenge_data_msg, (void *) (send_buf + 3));  //pack message into the rest of send_buf (starting at send_buf[3] onward)
	
	//send message on socket
	if (writen(*connfd, send_buf, (size_t) (len_pb + 3)) == -1) {
		error("write: sending challenge data message failed @shep");
	}
	
	//free all allocated memory
	free(challenge_data_msg.payload[0]);
	free(send_buf);
}

void recv_new_msg (int *connfd, FILE *results_fp, Text *challenge_data_msg, RunMode *run_mode_msg)
{
	net_msg_t msg_type;     //message type
	uint16_t len_pb;        //length of incoming serialized protobuf message
	uint8_t buf[MAX_SIZE];  //buffer to read raw data into
	int challenge_len;      //length of the challenge data received string
	
	//read one byte -> determine message type
	if (readn(*connfd, &msg_type, 1) < 0) {
		error("read: received EOF when attempting to get msg type @shep");
	}
	
	//read two bytes -> determine message length
	if (readn(*connfd, &len_pb, 2) < 0) {
		error("read: received EOF when attempting to get msg len @shep");
	}
	
	//read msg_len bytes -> put into buffer
	if (readn(*connfd, buf, (size_t) len_pb) < 0) {
		error("read: received EOF when attempting to get msg @shep");
	}
	
	//unpack according to message
	if (msg_type == CHALLENGE_DATA) {
		challenge_data_msg = text__unpack(NULL, len_pb, buf);
		
		//write the provided input data for the challenge to results.txt (results_fp)
		challenge_len = strlen(challenge_data_msg->payload[0]);
		if (fwrite(challenge_data_msg->payload[0], sizeof(char), challenge_len, results_fp) <= 0) {
			error("fwrite: could not write to results.txt for challenge data @shep");
		}
	} else if (msg_type == RUN_MODE) {
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
				robot_desc_write(RUN_MODE, CHALLENGE); //TODO: not implemented yet
				break;
			default:
				log_runtime(WARN, "requested robot to enter unknown robot mode");
		}
	} else {
		log_printf(WARN, "unknown message type %d; shepherd should only send RUN_MODE (2) or CHALLENGE_DATA (3)", msg_type);
	}
}

void *shepherd_conn (void *args)
{
	int *connfd = (int *) args;
	
	//initialize the protobuf messages
	Text log_msg = TEXT__INIT;
	log_msg.payload = (char **) malloc(sizeof(char *) * MAX_NUM_LOGS);
	
	Text challenge_data_msg = TEXT__INIT;
	challenge_data_msg.payload = (char **) malloc(sizeof(char *) * 1);
	challenge_data_msg.n_payload = 1; 
	
	RunMode run_mode_msg = RUN_MODE__INIT;
	
	//Open up log file and challenge results file; seek to the end of each file
	FILE *log_fp, *results_fp;
	if ((log_fp = fopen("../logger/runtime_log.log", "r")) == NULL) {
		error("fopen: could not open log file for reading @shep");
		return NULL;
	}
	if (fseek(log_fp, 0, SEEK_END) != 0) {
		error("fseek: could not seek to end of log file @shep");
		return NULL;
	}
	if ((results_fp = fopen("../executor/results.txt", "r+b")) == NULL) { //TODO: NOT IMPLEMENTED YET
		error("fopen: could not open results file for reading @shep");
		return NULL;
	}
	if (fseek(results_fp, 0, SEEK_END) != 0) { //TODO: NOT IMPLEMENTED YET
		error("fssek: could not seek to end of results file @shep");
		return NULL;
	}
	
	//variables used for waiting for something to do using select()
	int maxfdp1 = max(max(fileno(log_fp), fileno(results_fp)), *connfd) + 1;
	fd_set read_set;
	FD_ZERO(&read_set);
	
	//set cleanup objects for cancellation handler
	cleanup_objs_t cleanup_objs = { *connfd, log_fp, results_fp, &log_msg, &challenge_data_msg };
	
	//main control loop that is responsible for sending and receiving data
	//TODO: maybe try using pselect instead of select
	while (1) {
		FD_SET(fileno(log_fp), &read_set);
		FD_SET(connfd, &read_set);
		if (robot_desc_read(RUN_MODE) == CHALLENGE) {
			FD_SET(results_fp, &read_set);
		}
		
		//prepare to accept cancellation requests over the select
		pthread_cleanup_push(shep_conn_cleanup, (void *) cleanup_objs);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_testcancel();
		
		//wait for something to happen
		if (select(maxfdp1, &rset, NULL, NULL, NULL) < 0) {
			error("select: @shep");
		}
		
		//deny all cancellation requests until the next loop
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		pthread_cleanup_pop(0); //definitely do NOT execute the cleanup handler!
		
		//send a new log message if one is available
		if (FD_ISSET(fileno(log_fp), &read_set)) {
			send_log_msg(connfd, log_fp, &log_msg);
		}
		//send the challenge results if it is ready
		if (FD_ISSET(fileno(results_fp), &read_set)) {
			send_challenge_msg(connfd, results_fp, &challenge_data_msg);
		}
		//receive new message on socket if it is ready
		if (FD_ISSET(*connfd, &read_set)) {
			recv_new_msg(connfd, results_fp, &challenge_data_msg, &run_mode_msg);
			if (challenge_data_msg == NULL && run_mode_msg == NULL) { //shepherd disconnected
				break;
			}
		}
	}
	
	//call the cleanup function
	shep_conn_cleanup((void *) cleanup_objs);
	return NULL;
}

void start_shepherd_conn (int connfd)
{
	//create thread, give it connfd, write that shepherd is connected
	if (pthread_create(&shep_tid, NULL, shepherd_conn, &connfd) != 0) {
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