#include "net_util.h"

int sockfd = -1;

void sigint_handler (int signum)
{
	if (sockfd != -1) {
		close(sockfd);
	}
	sleep(2);
	exit(0);
}

int main ()
{
	struct sockaddr_in serv_addr, cli_addr;
	
	signal(SIGINT, &sigint_handler); //set the signal handler for SIGINT
	
	//create socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket: failed to create listening socket");
		return 1;
	}
	
	//set the elements of cli_addr
	memset(&cli_addr, '\0', sizeof(struct sockaddr_in));     //initialize everything to 0
	cli_addr.sin_family = AF_INET;                           //use IPv4
	cli_addr.sin_port = htons(DAWN_PORT);                    //use this port to connect
	inet_pton(AF_INET, DAWN_ADDR, &cli_addr.sin_addr);       //192.168.0.25 is my mac's address on the local network
	
	//bind the client side too, so that net_handler can verify it's shepherd
	if ((bind(sockfd, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr_in))) != 0) {
		perror("bind: failed to bind listening socket to raspi port");
		close(sockfd);
		return 1;
	} else {
		printf("bind: successfully bound listening socket to raspi port\n");
	}
	
	//set the elements of serv_addr
	memset(&serv_addr, '\0', sizeof(struct sockaddr_in));     //initialize everything to 0
	serv_addr.sin_family = AF_INET;                           //use IPv4
	serv_addr.sin_port = htons(RASPI_PORT);                   //want to connect to raspi port
	inet_pton(AF_INET, "192.168.0.24", &serv_addr.sin_addr);  //192.168.0.24 is the raspi IP address
	
	//connect to the server
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		perror("connect: failed to connect to socket");
	}
	
	//tell the executor to go into TELEOP mode (ideally)
	RunMode run_mode = RUN_MODE__INIT;
	unsigned len_pb;           //length of serialized protobuf message
	uint16_t len_pb_uint16;    //length of serialized protobuf message, as uin16_t
	uint8_t *send_buf;         //buffer for constructing the final log message
	
	run_mode.mode = MODE__AUTO;
	len_pb = run_mode__get_packed_size(&run_mode);
	send_buf = prep_buf(RUN_MODE_MSG, len_pb, &len_pb_uint16);
	run_mode__pack(&run_mode, (void *) (send_buf + 3));
	
	fprintf(stderr, "Writing TELEOP \n"); // See the length of message
	writen(sockfd, send_buf, (len_pb_uint16 + 3)); //write the message to the raspi
	
	free(send_buf); // Free the allocated serialized buffer
	
	//vars to read logs into
	net_msg_t msg_type;
	uint8_t buf[MAX_SIZE_BYTES]; //maximum log message size
	Text *log_msg;
	
	//do actions
	while (1) {
		//parse message 
		if (parse_msg(&sockfd, &msg_type, &len_pb_uint16, buf) != 0) {
			printf("raspi disconnected\n");
			break;
		}
		//unpack the message
		if ((log_msg = text__unpack(NULL, len_pb_uint16, buf)) == NULL) {
			printf("error unpacking incoming log message\n");
		}
		
		// display the message's fields.
		for (int i = 0; i < log_msg->n_payload; i++) {
			printf("%s", log_msg->payload[i]);
		}

		// Free the unpacked message
		text__free_unpacked(log_msg, NULL);
	}
	
	close(sockfd);
	
	sleep(2);
	
	return 0;
}
