#include "net_util.h"

int sockfd = -1;

void sigint_handler (int signum)
{
	if (sockfd != -1) {
		close(sockfd);
	}
	exit(0);
}

int main ()
{
	struct sockaddr_in serv_addr, cli_addr;
	
	signal(SIGINT, &sigint_handler); //set the signal handler for SIGINT
	logger_init(TEST);
	//create socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket: failed to create listening socket");
		return 1;
	}

	int optval = 1;
	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int))) != 0) {
		perror("setsockopt");
		printf("failed to set listening socket for reuse of port");
	}
	
	//set the elements of cli_addr
	memset(&cli_addr, '\0', sizeof(struct sockaddr_in));     //initialize everything to 0
	cli_addr.sin_family = AF_INET;                           //use IPv4
	cli_addr.sin_port = htons(DAWN_PORT);                    //use this port to connect
	cli_addr.sin_addr.s_addr = inet_addr(DAWN_ADDR);
	
	//bind the client side too, so that net_handler can verify it's Dawn
	if ((bind(sockfd, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr_in))) != 0) {
		perror("bind: failed to bind listening socket to client port");
		close(sockfd);
		return 1;
	} else {
		printf("bind: successfully bound listening socket to client port\n");
	}
	
	//set the elements of serv_addr
	memset(&serv_addr, '\0', sizeof(struct sockaddr_in));     //initialize everything to 0
	serv_addr.sin_family = AF_INET;                           //use IPv4
	serv_addr.sin_port = htons(RASPI_PORT);                   //want to connect to raspi port
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	//connect to the server
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		perror("connect: failed to connect to socket");
		return 1;
	}
	
	//tell the executor to go into TELEOP mode (ideally)
	RunMode run_mode = RUN_MODE__INIT;
	
	run_mode.mode = MODE__AUTO;
	uint16_t len_pb = run_mode__get_packed_size(&run_mode);
	uint8_t *send_buf = make_buf(RUN_MODE_MSG, len_pb);
	run_mode__pack(&run_mode, send_buf + 3);
	printf("buffer size %d \n", len_pb);
	fprintf(stderr, "Writing mode \n"); // See the length of message
	writen(sockfd, send_buf, len_pb + 3); //write the message to the raspi
	
	free(send_buf); // Free the allocated serialized buffer
	
	//vars to read logs into
	net_msg_t msg_type;
	uint8_t* buf; //maximum log message size
	Text *log_msg;
	uint16_t len;

	//do actions
	while (1) {
		//parse message 
		if (parse_msg(sockfd, &msg_type, &len, &buf) == 0) {
			printf("raspi disconnected\n");
			break;
		}
		//unpack the message
		if ((log_msg = text__unpack(NULL, len, buf)) == NULL) {
			printf("error unpacking incoming log message\n");
		}
		
		// display the message's fields.
		for (int i = 0; i < log_msg->n_payload; i++) {
			printf("%s", log_msg->payload[i]);
		}
		free(buf);
		// Free the unpacked message
		text__free_unpacked(log_msg, NULL);
	}
	
	close(sockfd);
		
	return 0;
}
