#include <arpa/inet.h>  // for inet_addr, bind, listen, accept, socket types
#include <netinet/in.h> // for structures relating to IPv4 addresses
#include <netdb.h>      // for struct addrinfo
#include "virtual_device_util.h"

/* Localhost; "host.docker.internal" is how to reference localhost from a Docker container
 * getent hosts host.docker.internal | awk '{ print $1 }'
 * Is used to resolve the IP address of this host name
 * https://unix.stackexchange.com/a/20793
 */
#define LOCALHOST "192.168.65.2"
#define PORT 5005 // This port must be exposed in docker-compose.yml

/**
 * Creates a socket and attempts to connect to host machine LOCALHOST
 * Returns:
 *    A valid socket file descriptor, or
 *    -1 on failure
 */
int connect_tcp() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("ERROR: connect_tcp: Couldn't create socket--%s\n", strerror(errno));
        return -1;
    }
    printf("Created TCP socket\n");

    // Assign IP and PORT to the socket
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(LOCALHOST);
    servaddr.sin_port = htons(PORT);

    // Connect the created socket to the server's socket
    if (connect(fd, (struct sockaddr*) &servaddr, sizeof(servaddr)) != 0) {
        printf("ERROR: connect_tcp: Couldn't connect to server--%s\n", strerror(errno));
        return -1;
    }
    printf("Successfully connected to server\n");
    return fd;
}

int main(int argc, char* argv[]) {
    int fd = connect_tcp();
    printf("TCP fd is %d\n", fd);
    close(fd);
    printf("Closed TCP socket\n");
    return 0;
}
