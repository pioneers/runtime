#include "net_handler_client.h"

//throughout this code, net_handler is abbreviated "nh"
pid_t nh_pid;                  //holds the pid of the net_handler process
pthread_t dump_tid;            //holds the thread id of the output dumper threads

int nh_tcp_shep_fd = -1;       //holds file descriptor for TCP Shepherd socket
int nh_tcp_dawn_fd = -1;       //holds file descriptor for TCP Dawn socket
int nh_udp_fd = -1;            //holds file descriptor for UDP Dawn socket
FILE *output_fp = NULL;        //holds current output location of client
FILE *null_fp = NULL;          //file pointer to /dev/null

// ************************************* HELPER FUNCTIONS ************************************** //

// Returns the number of milliseconds since the Unix Epoch
static uint64_t millis() {
	struct timeval time; // Holds the current time in seconds + microsecondsx
	gettimeofday(&time, NULL);
	uint64_t s1 = (uint64_t)(time.tv_sec) * 1000;  // Convert seconds to milliseconds
	uint64_t s2 = (time.tv_usec / 1000);		   // Convert microseconds to milliseconds
	return s1 + s2;
}

static int connect_tcp (int client)
{
	struct sockaddr_in serv_addr, cli_addr;
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("socket: failed to create listening socket: %s\n", strerror(errno));
		stop_net_handler();
		exit(1);
	}

	int optval = 1;
	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int))) != 0) {
		printf("setsockopt: failed to set listening socket for reuse of port: %s\n", strerror(errno));
	}
	
	//set the elements of cli_addr
	memset(&cli_addr, '\0', sizeof(struct sockaddr_in));     //initialize everything to 0
	cli_addr.sin_family = AF_INET;                           //use IPv4
	cli_addr.sin_port = (client == SHEPHERD_CLIENT) ? htons(SHEPHERD_PORT) : htons(DAWN_PORT); //use specifid port to connect
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);            //use any address set on this machine to connect
	
	//bind the client side too, so that net_handler can verify it's the proper client
	if ((bind(sockfd, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr_in))) != 0) {
		printf("bind: failed to bind listening socket to client port: %s\n", strerror(errno));
		close(sockfd);
		stop_net_handler();
		exit(1);
	}
	
	//set the elements of serv_addr
	memset(&serv_addr, '\0', sizeof(struct sockaddr_in));     //initialize everything to 0
	serv_addr.sin_family = AF_INET;                           //use IPv4
	serv_addr.sin_port = htons(RASPI_PORT);                   //want to connect to raspi port
	serv_addr.sin_addr.s_addr = inet_addr(RASPI_ADDR);
	
	//connect to the server
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in)) != 0) {
		printf("connect: failed to connect to socket: %s\n", strerror(errno));
		close(sockfd);
		stop_net_handler();
		exit(1);
	}
	
	//send the verification byte
	writen(sockfd, &client, 1);
	
	return sockfd;
}

static void recv_udp_data (int udp_fd)
{
	int max_size = 4096;
	uint8_t msg[max_size];
	struct sockaddr_in recvaddr;
	socklen_t addrlen = sizeof(recvaddr);
	int recv_size;
	
	//receive message from udp socket
	if ((recv_size = recvfrom(udp_fd, msg, max_size, 0, (struct sockaddr*) &recvaddr, &addrlen)) < 0) {
		fprintf(output_fp, "recvfrom: %s\n", strerror(errno));
	}
	fprintf(output_fp, "Raspi IP is %s:%d\n", inet_ntoa(recvaddr.sin_addr), ntohs(recvaddr.sin_port));
	fprintf(output_fp, "Received data size %d\n", recv_size);
	DevData* dev_data = dev_data__unpack(NULL, recv_size, msg);
	if (dev_data == NULL) {
		printf("Error unpacking incoming message\n");
	}
	
	// display the message's fields.
	fprintf(output_fp, "Received:\n");
	for (int i = 0; i < dev_data->n_devices; i++) {
		fprintf(output_fp, "Device No. %d: ", i);
		fprintf(output_fp, "\ttype = %s, uid = %llu, itype = %d\n", dev_data->devices[i]->name, dev_data->devices[i]->uid, dev_data->devices[i]->type);
		fprintf(output_fp, "\tParams:\n");
		for (int j = 0; j < dev_data->devices[i]->n_params; j++) {
			fprintf(output_fp, "\t\tparam \"%s\" has type ", dev_data->devices[i]->params[j]->name);
			switch (dev_data->devices[i]->params[j]->val_case) {
				case (PARAM__VAL_FVAL):
					fprintf(output_fp, "FLOAT with value %f\n", dev_data->devices[i]->params[j]->fval);
					break;
				case (PARAM__VAL_IVAL):
					fprintf(output_fp, "INT with value %d\n", dev_data->devices[i]->params[j]->ival);
					break;
				case (PARAM__VAL_BVAL):
					fprintf(output_fp, "BOOL with value %d\n", dev_data->devices[i]->params[j]->bval);
					break;
				default:
					fprintf(output_fp, "ERROR: no param value");
					break;
			}
		}
	}
	
	// Free the unpacked message
	dev_data__free_unpacked(dev_data, NULL);
	fflush(output_fp);
}

static int recv_tcp_data (int client, int tcp_fd)
{
	//variables to read messages into
	Text* msg;
	net_msg_t msg_type;
	uint8_t *buf;
	uint16_t len;
	char client_str[16];
	if (client == SHEPHERD_CLIENT) {
		strcpy(client_str, "SHEPHERD");
	} else {
		strcpy(client_str, "DAWN");
	}

	fprintf(output_fp, "From %s:\n", client_str);
	//parse message
	if (parse_msg(tcp_fd, &msg_type, &len, &buf) == 0) {
		printf("Net handler disconnected\n");
		return -1;
	}
	
	//unpack the message
	if ((msg = text__unpack(NULL, len, buf)) == NULL) {
		fprintf(output_fp, "Error unpacking incoming message from %s\n", client_str);
	}
	
	//print the incoming message
	if (msg_type == LOG_MSG) {
		for (int i = 0; i < msg->n_payload; i++) {
			fprintf(output_fp, "%s", msg->payload[i]);
		}
	} else if (msg_type == CHALLENGE_DATA_MSG) {
		for (int i = 0; i < msg->n_payload; i++) {
			fprintf(output_fp, "Challenge %d result: %s\n", i, msg->payload[i]);
		}
	}
	fflush(output_fp);
	
	//free allocated memory
	free(buf);
	text__free_unpacked(msg, NULL);
	
	return 0;
}

//dumps output from net handler stdout to this process's standard out
static void *output_dump (void *args)
{
	const int sample_size = 4; //number of messages that need to come in before disabling output
	const uint64_t disable_threshold = 50; //if the interval between each of the past sample_size messages has been less than this many milliseconds, disable output
	const uint64_t enable_threshold = 1000; //if this many milliseconds have passed between now and last received message, enable output
	uint64_t last_received_time = 0, curr_time;
	uint32_t less_than_disable_thresh = 0;
	
	fd_set read_set;
	int maxfd = (nh_tcp_dawn_fd > nh_tcp_shep_fd) ? nh_tcp_dawn_fd : nh_tcp_shep_fd;
	maxfd = (nh_udp_fd > maxfd) ? nh_udp_fd : maxfd;
	maxfd++;
	
	//wait for net handler to create some output, then print that output to stdout of this process
	while (1) {
		//set up the read_set argument to selct()
		FD_ZERO(&read_set);
		FD_SET(nh_tcp_shep_fd, &read_set);
		FD_SET(nh_tcp_dawn_fd, &read_set);
		FD_SET(nh_udp_fd, &read_set);
		
		//prepare to accept cancellation requests over the select
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		
		//wait for something to happen
		if (select(maxfd, &read_set, NULL, NULL, NULL) < 0) {
			printf("select: output dump: %s\n", strerror(errno));
		}
		
		//deny all cancellation requests until the next loop
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		
		//enable output if more than enable_thresh has passed between last time and previous time
		curr_time = millis();
		if (curr_time - last_received_time >= enable_threshold) {
			less_than_disable_thresh = 0;
			output_fp = stdout;
		}
		if (curr_time - last_received_time <= disable_threshold) {
			less_than_disable_thresh++;
			if (less_than_disable_thresh == sample_size) {
				printf("Suppressing output: too many messages...\n\n");
				fflush(stdout);
				output_fp = null_fp;
			}
		}
		last_received_time = curr_time;

		//print stuff from whichever file descriptors are ready for reading...
		if (FD_ISSET(nh_tcp_shep_fd, &read_set)) {
			if (recv_tcp_data(SHEPHERD, nh_tcp_shep_fd) == -1) {
				return NULL;
			}
		}
		if (FD_ISSET(nh_tcp_dawn_fd, &read_set)) {
			if (recv_tcp_data(DAWN, nh_tcp_dawn_fd) == -1) {
				return NULL;
			}
		}
		if (FD_ISSET(nh_udp_fd, &read_set)) {
			recv_udp_data(nh_udp_fd);
		}
	}
	return NULL;
}

// ************************************* NET HANDLER CLIENT FUNCTIONS ************************** //

void start_net_handler ()
{

	//fork net_handler process
	if ((nh_pid = fork()) < 0) {
		printf("fork: %s\n", strerror(errno));
	} else if (nh_pid == 0) { //child
		//exec the actual net_handler process
		if (execlp("../../net_handler/net_handler", "net_handler", (char *) 0) < 0) {
			printf("execlp: %s\n", strerror(errno));
		}
	} else { //parent
		sleep(1); //allows net_handler to set itself up

		//Connect to the raspi networking ports to catch network output
		nh_tcp_dawn_fd = connect_tcp(DAWN_CLIENT);
		nh_tcp_shep_fd = connect_tcp(SHEPHERD_CLIENT);
		if ((nh_udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
			printf("socket: UDP socket creation failed...\n");
			stop_net_handler();
			exit(1);
		}
		//open /dev/null
		null_fp = fopen("/dev/null", "w");

		//start the thread that is dumping output from net_handler to stdout of this process
		if (pthread_create(&dump_tid, NULL, output_dump, NULL) != 0) {
			printf("pthread_create: output dump\n");
		}
		sleep(1); //allow time for thread to dump output before returning to client
	}
}

void stop_net_handler ()
{
	//send signal to net_handler and wait for termination
	if (kill(nh_pid, SIGINT) < 0) {
		printf("kill: %s\n", strerror(errno));
	}
	if (waitpid(nh_pid, NULL, 0) < 0) {
		printf("waitpid: %s\n", strerror(errno));
	}
	
	//killing net handler should cause dump thread to return, so join with it
	if (pthread_join(dump_tid, NULL) != 0) {
		printf("pthread_join: output dump\n");
	}
	
	//close all the file descriptors
	if (nh_tcp_shep_fd != -1) {
		close(nh_tcp_shep_fd);
	}
	if (nh_tcp_dawn_fd != -1) {
		close(nh_tcp_dawn_fd);
	}
	if (nh_udp_fd != -1) {
		close(nh_udp_fd);
	}
}	
	

void send_run_mode (int client, int mode)
{
	RunMode run_mode = RUN_MODE__INIT;
	uint8_t *send_buf;
	uint16_t len;
	
	//set the right mode
	switch (mode) {
		case (IDLE_MODE):
			run_mode.mode = MODE__IDLE;
			break;
		case (AUTO_MODE):
			run_mode.mode = MODE__AUTO;
			break;
		case (TELEOP_MODE):
			run_mode.mode = MODE__TELEOP;
			break;
		default:
			printf("ERROR: sending run mode message\n");
	}
	
	//build the message
	len = run_mode__get_packed_size(&run_mode);
	send_buf = make_buf(RUN_MODE_MSG, len);
	run_mode__pack(&run_mode, send_buf + 3);
	
	//send the message
	if (client == SHEPHERD_CLIENT) {
		writen(nh_tcp_shep_fd, send_buf, len + 3);
	} else {
		writen(nh_tcp_dawn_fd, send_buf, len + 3);
	}
	free(send_buf);
	sleep(1); //allow time for net handler and runtime to react and generate output before returning to client
}

void send_start_pos (int client, int pos)
{
	StartPos start_pos = START_POS__INIT;
	uint8_t *send_buf;
	uint16_t len;
	
	//set the right mode
	switch (pos) {
		case (LEFT_POS):
			start_pos.pos = POS__LEFT;
			break;
		case (RIGHT_POS):
			start_pos.pos = POS__RIGHT;
			break;
		default:
			printf("ERROR: sending run mode message\n");
	}
	
	//build the message
	len = start_pos__get_packed_size(&start_pos);
	send_buf = make_buf(START_POS_MSG, len);
	start_pos__pack(&start_pos, send_buf + 3);
	
	//send the message
	if (client == SHEPHERD_CLIENT) {
		writen(nh_tcp_shep_fd, send_buf, len + 3);
	} else {
		writen(nh_tcp_dawn_fd, send_buf, len + 3);
	}
	free(send_buf);
	sleep(1); //allow time for net handler and runtime to react and generate output before returning to client
}

void send_challenge_data (int client, char **data)
{
	
}

void send_device_data (dev_data_t *data)
{
	
}
