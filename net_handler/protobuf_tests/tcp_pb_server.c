#include <arpa/inet.h>  //for inet_addr, bind, listen, accept, socket types
#include <stdio.h>      //for i/o
#include <stdlib.h>     //for malloc, free, exit
#include <string.h>     //for strcpy, memset
#include <unistd.h>     //for read, write, close

#include "../pbc_gen/text.pb-c.h"

#define PORT 5001

#define MAX_STRLEN 100

char* strs[4] = {"hello", "beautiful", "precious", "world"};

/* NOTES:
 * this is the server.
 * to read from a connection, you read from the file descriptor of the connection socket
 * to write to a connection, you write to the file descriptor of the connection socket
 * in this file, that file descriptor is called connfd
 */

int main() {
    Text log_msg = TEXT__INIT;  //iniitialize hooray
    uint8_t* log_msg_buf;       // Buffer to store serialized data
    char conn_msg_buf[100];     //buffer for connection message
    unsigned len;               // Length of serialized data

    //put some data
    log_msg.n_payload = 4;
    log_msg.payload = (char**) malloc(sizeof(char*) * log_msg.n_payload);
    for (int i = 0; i < log_msg.n_payload; i++) {
        log_msg.payload[i] = (char*) malloc(sizeof(char) * strlen(strs[i]));
        strcpy(log_msg.payload[i], (const char*) strs[i]);
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
    if ((bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        perror("bind");
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
    connfd = accept(sockfd, (struct sockaddr*) &cli, &len);
    if (connfd < 0) {
        printf("server acccept failed...\n");
        exit(0);
    } else {
        printf("server acccept the client...\n");
    }

    //read a connection message for example
    ssize_t n = read(connfd, conn_msg_buf, 255);
    if (n < 0) {
        perror("ERROR reading from socket");
    }
    printf("Here is the message: %s\n", conn_msg_buf);

    //send the serialized data to the client
    len = text__get_packed_size(&log_msg);
    fprintf(stderr, "Writing %d serialized bytes\n", len);  // See the length of message
    log_msg_buf = (uint8_t*) malloc(sizeof(uint8_t) * len);
    text__pack(&log_msg, log_msg_buf);
    ssize_t w_bytes = write(connfd, log_msg_buf, len);
    printf("wrote %zd bytes to client\n", w_bytes);
    if (w_bytes == -1) {
        perror("failed in socket write");
    }
    // After chatting close the socket
    close(connfd);
    close(sockfd);

    //fwrite(buf, len, 1, stdout); // Write to stdout to allow direct command line piping

    free(log_msg_buf);  // Free the allocated serialized buffer
    free(log_msg.payload);
    return 0;
}