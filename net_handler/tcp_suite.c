#include "tcp_suite.h"

//send LOG messages to shepherd and dawn
void *tcp_send_thread (void *args)
{	
	char *buf = (char *) malloc(sizeof(char) * log_msg_maxlen);
	log_payload_t log_payload;
	text_payload_t text_payload;
	log_payload.msg = LOG;
	text_payload.msg = CODE_UPLOAD_REQUEST;
	
	//open log file and go to the end
	FILE *fp;
	fp = fopen("../logger/runtime_log.log", "r");
	fseek(fp, 0, SEEK_END);
	
	//TODO: add condition variable to stop thread
	while (1) {
		log_payload.num_logs = 0;
		while (log_payload.num_logs < MAX_NUM_LOGS) {
			if (fgets(buf, LOG_MSG_MAXLEN, fp) != NULL) {
				strcpy(log_payload.logs[log_payload.num_logs], (const char *) buf);
				log_payload.num_logs++;
			} else {
				break;
			}
			usleep(1000); //wait a bit so we aren't going as fast as possible
		}
		//TODO: package data and send over TCP socket
	}
}

//receive RUN_MODE, CODE_UPLOAD_REQUEST over TCP
void *tcp_recv_thread (void *args)
{
	text_payload_t payload;
	
	while (1) {
		
	}
}



//start a trio of threads managing a TCP connection
void start_tcp_suite (char *port)
{
	//load argument and prepare to start; create any synchronization variables necessary
	//create sender thread
	//create receiver thread
	//create manage thread
}

//stop the trio of threads managing the TCP connection
void stop_tcp_suite (char *port)
{
	//signal to relay thread that it needs to stop the other threads
	//join with all of them (or until timeout)
	//return
}