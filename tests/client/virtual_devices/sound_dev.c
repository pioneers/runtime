/**
 * SoundDevice, a proper lowcar device that plays sounds.
 * Students write a floating point pitch to the PITCH parameter to play it.
 * SoundDevice writes to a socket to sound.py, which plays the sound on the host machine
 */
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

// SoundDevice params
enum {
    SOCKFD,
    PITCH
};

/**
 * Initialize the values for each param
 * Arguments:
 *    params: Array of params to be initialized
 */
void init_params(param_val_t params[]) {
    params[SOCKFD].p_i = -1;
    params[PITCH].p_f = 0;
}

/**
 * Sends a pitch to be played
 * Arguments:
 *    sockfd: The file descriptor of the socket to write to
 *    pitch: The pitch to be sent to the socket
 * Returns:
 *    the number of bytes written to the socket
 */
int send_pitch(int sockfd, float pitch) {
    printf("send_pitch: Sending %f to socket\n", pitch);
    fflush(stdout);
    return write(sockfd, (uint8_t*) &pitch, sizeof(float));
}

/**
 * Arguments:
 *    params: Array of param values to be modified
 */
void device_actions(param_val_t params[]) {
    send_pitch(params[SOCKFD].p_i, params[PITCH].p_f);
}

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
    return fd;
}

/**
 * A device that behaves like a lowcar device, connected to dev handler via a socket
 * Arguments:
 *    int: file descriptor for the socket
 *    uint64_t: device uid
 */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Incorrect number of arguments: %d out of %d\n", argc, 3);
        exit(1);
    }

    int fd = atoi(argv[1]);
    uint64_t uid = strtoull(argv[2], NULL, 0);

    uint8_t dev_type = device_name_to_type("SoundDevice");
    device_t* dev = get_device(dev_type);

    param_val_t params[dev->num_params];
    init_params(params);

    // Connect to sound.py
    params[SOCKFD].p_i = connect_tcp();

    lowcar_protocol(fd, dev_type, dev_type, uid, params, &device_actions, -1);
    close(fd);
    return 0;
}
