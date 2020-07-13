#include "../net_util.h"

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
	//logger_init(TEST);
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
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
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
	serv_addr.sin_addr.s_addr = inet_addr(RASPI_ADDR);
	
	//connect to the server
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		perror("connect: failed to connect to socket");
		return 1;
	}

	uint8_t client = 1;
	write(sockfd, &client, 1);
	printf("send client type\n");
	
	//tell the executor to go into TELEOP mode
	RunMode run_mode = RUN_MODE__INIT;
	
	run_mode.mode = MODE__AUTO;
	uint16_t len_pb = run_mode__get_packed_size(&run_mode);
	printf("buffer size %d \n", len_pb);
	uint8_t *send_buf = make_buf(RUN_MODE_MSG, len_pb);
	run_mode__pack(&run_mode, send_buf + 3);
	writen(sockfd, send_buf, len_pb + 3); //write the message to the raspi
	free(send_buf); // Free the allocated serialized buffer
	
	sleep(3);
	Text inputs = TEXT__INIT;
	inputs.n_payload = NUM_CHALLENGES;
	inputs.payload = malloc(sizeof(char*) * inputs.n_payload);
	inputs.payload[0] = "2039";
	inputs.payload[1] = "190172344";
	len_pb = text__get_packed_size(&inputs);
	send_buf = make_buf(CHALLENGE_DATA_MSG, len_pb);
	text__pack(&inputs, send_buf+3);
	printf("Giving challenge inputs \n");
	writen(sockfd, send_buf, len_pb + 3);
	free(send_buf);

	//vars to read logs into
	net_msg_t msg_type;
	uint8_t* buf; //maximum log message size
	Text* msg;
	uint16_t len;

	int size = 30;
	char mode_str[size];
	fd_set read_set;
	int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
	maxfd = maxfd + 1;

	//do actions
	while (1) {
		FD_ZERO(&read_set);
		FD_SET(sockfd, &read_set);
		FD_SET(STDIN_FILENO, &read_set);

		if (select(maxfd, &read_set, NULL, NULL, NULL) < 0) {
			printf("Failed to wait for select: %s", strerror(errno));
			break;
		}

		if (FD_ISSET(sockfd, &read_set)) {
			//parse message 
			if (parse_msg(sockfd, &msg_type, &len, &buf) == 0) {
				printf("raspi disconnected\n");
				break;
			}
			//unpack the message
			if ((msg = text__unpack(NULL, len, buf)) == NULL) {
				printf("error unpacking incoming log message\n");
			}
			
			if (msg_type == LOG_MSG) {
				// display the message's fields.
				for (int i = 0; i < msg->n_payload; i++) {
					printf("%s", msg->payload[i]);
				}
			}
			else if (msg_type == CHALLENGE_DATA_MSG) {
				for (int i = 0; i < msg->n_payload; i++) {
					printf("Challenge %d result: %s\n", i, msg->payload[i]);
				}
			}
			// Free allocated memory
			free(buf);
			text__free_unpacked(msg, NULL);
		}

		if (FD_ISSET(STDIN_FILENO, &read_set)) {
			fgets(mode_str, size, stdin);
			RunMode mode = RUN_MODE__INIT;
			if (strcmp(mode_str, "auto\n") == 0) {
				mode.mode = MODE__AUTO;
			}
			else if (strcmp(mode_str, "teleop\n") == 0) {
				mode.mode = MODE__TELEOP;
			}
			else if (strcmp(mode_str, "idle\n") == 0) {
				// mode.mode = MODE__IDLE;
			}
			else {
				continue;
			}
			len_pb = run_mode__get_packed_size(&mode);
			send_buf = make_buf(RUN_MODE_MSG, len_pb);
			run_mode__pack(&mode, send_buf + 3);
			printf("Protobuf length %d \n", len_pb);
			writen(sockfd, send_buf, len_pb + 3); //write the message to the raspi
			free(send_buf); // Free the allocated serialized buffer             
		}

	}


	
	close(sockfd);
		
	return 0;
}
