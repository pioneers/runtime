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
char global_buf[MAX_LOG_LEN];
int global_buf_pending = 0;    //to keep track of whether there's stuff in the glob_buf for processing

//cleanup function for dawn_conn thread
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

//TODO: try and figure out the most optimized way to set up the log message (maybe write a custom allocator?)
static void send_log_msg (int *connfd, int fifofd, Text *log_msg)
{
	uint8_t *send_buf;           //buffer to send data on
	unsigned len_pb;             //length of serialized protobuf
	uint16_t len_pb_uint16;      //length of serialized protobuf, as uint16_t
	size_t next_line_len;        //length of the next log line
	ssize_t n_read;              //return value of read(), normally the number of bytes read
	int log_msg_ix = 0;          //indexer into the string in log_msg
	int line_start_ix;           //saves index of start of current line
	char buf[MAX_LOG_LEN];       //buffer to read MAX_LOG_LEN bytes from the FIFO
	int i, j, k;                 //indexers (i is an indexer into global_buf, j into buf, k into log_msg->payload[log_msg->n_payload])
	
	//read in log lines until there are no more to read, or we read MAX_NUM_LOGS lines
	log_msg->n_payload = 0;
	while (log_msg->n_payload < MAX_NUM_LOGS) {
		n_read = read(fifofd, buf, MAX_LOG_LEN);
		if (n_read == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {  //if no more data on fifo pipe
				break;
			} else {  //some other error
				error("read: log fifo @dawn");
				break;
			}
		}
		
		//if there's something we need to take care of in the global buf
		i = 0, j = 0;
		if (global_buf_pending == 1) {
			for ( ; global_buf[i] != '\n'; i++);
			for ( ; buf[j] != '\n'; j++);
			log_msg->payload[log_msg->n_payload] = (char *) malloc(sizeof(char) * (i + j + 2));
			//copy what remains in the global buf
			for (k = 0; k < i; k++) {
				log_msg->payload[log_msg->n_payload][k] = global_buf[k];
			}
			//copy the rest of the message in buf
			for ( ; k < (i + j + 1); k++) {
				log_msg->payload[log_msg->n_payload][k] = buf[k - i];
			}
			log_msg->payload[log_msg->n_payload][k + 1] = '\0'; //null terminate
			log_msg->n_payload++;
			j++;
			global_buf_pending = 0;
		}
		
		//save start of line
		//read until newline or nothing left
		//if newline then malloc current index - start of line bytes and copy; increment n_payloads and repeat from top
		//if nothing left, continue and don't reset anything
		//after incrementing n_payloads, if over MAX_NUM_LOGS, then break and don't reset anything
		while (log_msg->n_payload < MAX_NUM_LOGS) {
			line_start_ix = j;
			for ( ; buf[j] != '\n' && j < n_read; j++);
			if (buf[j] == '\n') {
				log_msg->payload[log_msg->n_payload] = (char *) malloc(sizeof(char) * (j - line_start_ix + 2));
				//copy the line
				for (k = 0; k <= (j - line_start_ix); k++) {
					log_msg->payload[log_msg->n_payload][k] = buf[line_start_ix + k];
				}
				log_msg->payload[log_msg->n_payload][k + 1] = '\0'; //null terminate the line
				log_msg->n_payload++;
				j++;
			} else {
				//if we got to the end of buf, then stash the rest of buf into global_buf to prepend to result of next read
				for (k = 0; k <= (j - line_start_ix); k++) {
					global_buf[k] = buf[line_start_ix + k];
				}
				global_buf[k + 1] = '\n';
				global_buf_pending = 1;
				break; //go back to outer while loop and read more from the FIFO pipe
			}
		}
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
		error("write: sending log message failed @dawn");
	}
	
	//free all allocated memory
	for (int i = 0; i < log_msg->n_payload; i++) {
		free(log_msg->payload[i]);
	}
	free(send_buf);
}

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

void start_dawn_conn (int connfd)
{
	dawn_connfd = connfd;
	
	//create thread, give it connfd, write that dawn is connected
	if (pthread_create(&dawn_tid, NULL, dawn_conn, NULL) != 0) {
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