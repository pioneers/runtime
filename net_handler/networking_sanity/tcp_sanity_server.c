#include <stdio.h>       //for i/o
#include <stdlib.h>      //for malloc, free, exit
#include <string.h>      //for strcpy, memset
#include <arpa/inet.h>   //for inet_addr, bind, listen, accept, socket types
#include <unistd.h>      //for read, write, close

#include "../protobuf-c/text.pb-c.h"

#define PORT 8192

#define MAX_STRLEN 100

char* strs[4] = { "hello", "beautiful", "precious", "world" };

int main () 
{
	Text log_msg = TEXT__INIT; //iniitialize hooray
	uint8_t *log_msg_buf;                     // Buffer to store serialized data
	char conn_msg_buf[100]; //buffer for connection message
	unsigned len;                             // Length of serialized data
	
	//put some data
	log_msg.msg = MSG__LOG;   //this is how you do enums
	log_msg.n_payloads = 4;
	log_msg.payloads = (char **) malloc (sizeof(char *) * log_msg.n_payloads);
	for (int i = 0; i < log_msg.n_payloads; i++) {
		log_msg.payloads[i] = (char *) malloc(sizeof(char) * strlen(strs[i]));
		strcpy(log_msg.payloads[i], (const char *) strs[i]);
	}
	
	//establish TCP sever-side
	int sockfd, connfd; 
	struct sockaddr_in servaddr, cli; 
  
	// socket create and verification 
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
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(PORT); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	} else {
		printf("Socket successfully binded..\n"); 
	}
  
	// Now server is ready to listen and verification 
	if ((listen(sockfd, 5)) != 0) { 
		printf("Listen failed...\n"); 
		exit(0); 
	} else {
		printf("Server listening..\n"); 
	}
	len = sizeof(cli);
  
	// Accept the data packet from client and verification 
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len); 
	if (connfd < 0) { 
		printf("server acccept failed...\n"); 	
		exit(0); 
	} else {
		printf("server acccept the client...\n"); 
	}
	
	//send the serialized data to the client
	len = text__get_packed_size(&log_msg);
	fprintf(stderr, "Writing %d serialized bytes\n", len); // See the length of message
	log_msg_buf = (uint8_t *) malloc(sizeof(uint8_t) * len);
	text__pack(&log_msg, log_msg_buf);
	ssize_t w_bytes = write(connfd, log_msg_buf, len); 
	printf("wrote %zd bytes to client\n", w_bytes);
	if (w_bytes == -1) {
		perror("failed in socket write");
	}
	
	// After chatting close the socket 
    close(sockfd); 
	
	//fwrite(buf, len, 1, stdout); // Write to stdout to allow direct command line piping
	
	free(log_msg_buf); // Free the allocated serialized buffer
	free(log_msg.payloads);
	return 0;
}