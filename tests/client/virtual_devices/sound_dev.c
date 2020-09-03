#include <arpa/inet.h>   // for inet_addr, bind, listen, accept, socket types
#include <netinet/in.h>  // for structures relating to IPv4 addresses
#include "virtual_device_util.h"

// Localhost; "host.docker.internal" is how to reference localhost from a Docker container
#define LOCALHOST "host.docker.internal"
#define PORT 5005 // This port must be exposed in docker-compose.yml

int connect_tcp() {
    // Create TCP Socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("connect_tcp: Couldn't create socket: %s\n", strerror(errno));
        return -1;
    }

    // Assign IP and Port to the socket
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    addr.sin_port = htons(PORT);

    // Connect socket to server
    if (connect(sockfd, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
        printf("connect_tcp: Couldn't connect to server: %s\n", strerror(errno));
        return -1;
    }

    return sockfd;
}

int main(int argc, char* argv[]) {
    //logger_init(DEV_HANDLER);
    int fd = connect_tcp();
    printf("TCP fd is %d\n", fd);
    close(fd);
    return 0;
}
