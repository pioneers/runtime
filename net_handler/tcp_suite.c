#include "tcp_suite.h"

uint16_t ports[2][2] = { {8000, 8001} , {8100, 8101} }; //hard-coded port numbers
pthread_t td_ids[2][2];
td_arg_t global_td_args[2];

//structure of argument into a pair of tcp threads
typedef struct tcp_arg = {
	uint8_t challenge_wip;
	uint16_t send_port;
	uint16_t recv_port;
	uint8_t stop;
} tcp_arg_t;

static void error (char *msg)
{
	perror(msg);
	log_runtime(ERROR, msg); //send the message to the logger
	exit(1);
}

//send LOG messages to shepherd and dawn
void *tcp_send_thread (void *args)
{	
	tcp_arg_t *tcp_args = (tcp_arg_t *)args; //use this to communicate between the send and recv threads
	
	char *buf = (char *) malloc(sizeof(char) * log_msg_maxlen);
	log_payload_t log_payload;
	text_payload_t text_payload;
	log_payload.msg = LOG;
	text_payload.msg = CHALLENGE_DATA;
	
	//open log file and go to the end
	FILE *fp;
	fp = fopen("../logger/runtime_log.log", "r");
	fseek(fp, 0, SEEK_END);
	
	while (!(tcp_args->stop)) {
		if (fgets(buf, LOG_MSG_MAXLEN, fp) != NULL) { //if there's a log message to send
			log_payload.num_logs = 0;
			do {
				strcpy(log_payload.logs[log_payload.num_logs], (const char *)buf);
				log_payload_num_logs++;
			} while (fgets(buf, LOG_MSG_MAXLEN, fp) != NULL); //while there's still stuff to read, get it all
			//TODO: package data and send over TCP socket
		}
		if (tcp_args->challenge_wip) {
			if (/* TODO: check to see if coding challenge results are in */) {
				/* TODO: read in, package, encode, send over TCP port */
				tcp_args->challenge_wip = 0; //coding challenge not wip any more
			}
		}
		usleep(1000); //sleep a bit so we're not looping as fast as possible
	}
	return NULL;
}

//receive RUN_MODE, CODE_UPLOAD_REQUEST over TCP
void *tcp_recv_thread (void *args)
{
	tcp_arg_t *tcp_args = (tcp_arg_t *)args; //use this to communicate between the send and recv threads
	
	text_payload_t payload;
	payload.msg = NOP;
	
	while (!(tcp_args->stop)) {
		// TODO: receive a message from the port and load into payload (do NOT block here!)
		
		//handle the received packet
		if (payload.msg == RUN_MODE) {
			if (!strcmp(payload.text, "IDLE")) {
				log_runtime(DEBUG, "entering IDLE mode");
				robot_desc_write(RUN_MODE, IDLE);
			} else if (!strcmp(payload.text, "AUTO")) {
				log_runtime(DEBUG, "entering AUTO mode");
				robot_desc_write(RUN_MODE, AUTO);
			} else if (!strcmp(payload.text, "TELEOP")) {
				log_runtime(DEBUG, "entering TELEOP mode");
				robot_desc_write(RUN_MODE, TELEOP);
			} else if (!strcmp(payload.text, "CHALLENGE")) {
				log_runtime(DEBUG, "entering CHALLENGE mode; running coding challenges!");
				robot_desc_write(RUN_MODE, CHALLENGE); //TODO: won't compile as of now, need to check this
				tcp_args->challenge_wip = 1; //tell send thread to start waiting for coding challenge to finish
			} else {
				log_runtime(WARN, "requested robot to enter unknown robot mode");
			}
		} else if (payload.msg == CHALLENGE_DATA) {
			//TODO: take payload.text and put it somewhere for the executor to use when running challenges
		} else if (payload.msg != NOP) {
			log_runtime(WARN, "unknown packet type received on TCP");
		}
		payload.msg = NOP; //reset this
		usleep(1000); //sleep a bit so we're not looping as fast as possible
	}
	free(tcp_args); //recv thread is responsible for freeing the argument
	return NULL;
}

//start the threads managing a TCP connection
void start_tcp_suite (endpoint_t endpoint)
{
	tcp_arg_t td_args = (tcp_arg_t *) malloc(sizeof(tcp_arg_t));
	td_args->challenge_wip = 0;
	td_args->send_port = ports[endpoint][0];
	td_args->recv_port = ports[endpoint][1];
	td_args->stop = 0;
	char msg[64];     //for error messages
	
	//create send thread
	if (pthread_create(&td_ids[endpoint][0], NULL, tcp_send_thread, td_args) != 0) {
		sprintf(msg, "failed to create TCP send thread with endpoint %d", endpoint);
		error(msg);
	}
	
	//create recv thread
	if (pthread_create(&td_ids[endpoint][1], NULL, tcp_recv_thread, td_args) != 0) {
		sprintf(msg, "failed to create TCP recv thread with endpoint %d", endpoint);
		error(msg);
	}
	
	//save this for later telling the threads to stop
	global_td_args[endpoint] = td_args; 
}

//stop the threads managing the TCP connection
void stop_tcp_suite (endpoint_t endpoint)
{
	global_td_args[endpoint].stop = 1;
	
	//join with send thread
	if (pthread_join(td_ids[endpoint][0], NULL) != 0) {
		sprintf(msg, "failed to join TCP send thread with endpoint %d", endpoint);
		error(msg);
	}
	
	//join with recv thread
	if (pthread_join(td_ids[endpoint][1], NULL) != 0) {
		sprintf(msg, "failed to join TCP recv thread with endpoint %d", endpoint);
		error(msg);
	}
}