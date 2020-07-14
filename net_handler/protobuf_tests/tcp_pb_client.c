#include <stdio.h>       //for i/o
#include <stdlib.h>      //for exit
#include <string.h>      //for memset
#include <arpa/inet.h>   //for inet_addr, bind, listen, accept, socket types
#include <unistd.h>      //for read, write, close

#include "../pbc_gen/text.pb-c.h"
#define MAX_MSG_SIZE 1024
#define PORT 5001

/* NOTES:
 * this is the client.
 * to read from the server, you read from the file descriptor of my own socket
 * to write to a server, you write to the file descriptor of my own socket
 * in this file, that file descriptor is called sockfd
 */

int main () 
{
	uint8_t buf[MAX_MSG_SIZE];
	
	//Open TCP connection
	int sockfd; 
	struct sockaddr_in servaddr; 
  
	// socket create and varification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} else {
		printf("Socket successfully created..\n"); 
	}
	memset(&servaddr, '\0', sizeof(servaddr));
  
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	servaddr.sin_port = htons(PORT); 
  
	// connect the client socket to server socket 
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) { 
		printf("connection with the server failed...\n"); 
		exit(0); 
	} else {
		printf("connected to the server..\n"); 
	}
	
	//send a connection message for example
	ssize_t n = write(sockfd, "write", 6);
	if (n < 0) {
		perror("ERROR writing to socket");
	}
	
	//read the message
	ssize_t msg_len = read(sockfd, buf, 100); 
	printf("received %zd bytes from server\n", msg_len);
	
	// close the socket 
	close(sockfd); 
	
	//process the message
	Text *log_msg;
	
	// Unpack the message using protobuf-c.
	log_msg = text__unpack(NULL, (size_t) msg_len, buf);	
	if (log_msg == NULL) {
		fprintf(stderr, "error unpacking incoming message\n");
		exit(1);
	}
	
	// display the message's fields.
	for (int i = 0; i < log_msg->n_payload; i++) {
		printf("%s\n", log_msg->payload[i]);
	}
	// Free the unpacked message
	text__free_unpacked(log_msg, NULL);
	
	sleep(1);
	
	return 0;
}