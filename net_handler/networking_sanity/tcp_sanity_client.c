#include <stdio.h>       //for i/o
#include <stdlib.h>      //for exit
#include <string.h>      //for memset
#include <arpa/inet.h>   //for inet_addr, bind, listen, accept, socket types
#include <unistd.h>      //for read, write, close

#include "../protobuf-c/text.pb-c.h"
#define MAX_MSG_SIZE 1024
#define PORT 8192

int main () 
{
	uint8_t buf[MAX_MSG_SIZE];
	
	//Open TCP connection
	int sockfd, connfd; 
	struct sockaddr_in servaddr, cli; 
  
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
	printf("Received: msg = %u\n", log_msg->msg); //notice this comes out as 4, not MSG__LOG or some string
	for (int i = 0; i < log_msg->n_payloads; i++) {
		printf("\t%s\n", log_msg->payloads[i]);
	}

	// Free the unpacked message
	text__free_unpacked(log_msg, NULL);
	return 0;
}