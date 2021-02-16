#include "net_util.h"
#include "../runtime_util/runtime_util.h"

#define FWD_TCP_PORT 1234 // port to expose with ngrok to Dawn

int udp_fd = -1;
int tcp_fd = -1;

void start_udp_relay() {
    //create the socket
    if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        log_printf(ERROR, "start_udp_conn: could not create udp socket: %s", strerror(errno));
        return;
    }

    //bind the socket to any valid IP address on the raspi and specified port
    struct sockaddr_in my_addr = {0};  //for holding IP addresses (IPv4)
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(RASPI_ADDR);
    my_addr.sin_port = htons(RASPI_UDP_PORT);
}

void start_tcp_exposed_port() {
    struct sockaddr_in serv_addr = {0};  //initialize everything to 0

    //create socket
    if ((tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        log_printf(ERROR, "socket_setup: failed to create listening socket: %s", strerror(errno));
        return 1;
    }

    //set the socket option SO_REUSEPORT on so that if raspi terminates and restarts it can immediately reopen the same port
    int optval = 1;
    if ((setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int))) != 0) {
        log_printf(ERROR, "socket_setup: failed to set listening socket for reuse of port: %s", strerror(errno));
    }

    //set the elements of serv_addr
    serv_addr.sin_family = AF_INET;                 //use IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //use any IP interface on raspi
    serv_addr.sin_port = htons(FWD_TCP_PORT);       //assign a port number

    //bind socket to well-known IP_addr:port
    if ((bind(tcp_fd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in))) != 0) {
        log_printf(ERROR, "socket_setup: failed to bind listening socket to raspi port: %s", strerror(errno));
        close(tcp_fd);
        return 1;
    }

    //set the socket to be in listening mode (since the robot is the server)
    if ((listen(tcp_fd, 2)) != 0) {
        log_printf(ERROR, "socket_setup: failed to set listening socket to listen mode: %s", strerror(errno));
        close(tcp_fd);
        return 1;
    }
    return 0;
}

/* Daniel */ 
int udp_to_tcp() {
    
    // read from udp and get packed proto
    
    // get length of packed proto
    
    // attach length to packed proto
    
    // send data over tcp port
    
    return 0;
}

/* Vincent */
int tcp_to_udp() {
    
    // read first x number of bytes from incoming data on tcp port to get length
    
    // then read "length" bytes from tcp port to obtain data
    
    // send data immediately onto udp port
    
    return 0;
}

/* Ben */
void forward() {
    int ret;

    //variables used for waiting for something to do using select()
    fd_set read_set;
    int log_fd;
    int maxfd = (udp_fd > tcp_fd) ? udp_fd : tcp_fd;
    maxfd++;

    //main control loop that is responsible for sending and receiving data
    while (1) {
        //set up the read_set argument to selct()
        FD_ZERO(&read_set);
        FD_SET(udp_fd, &read_set);
        FD_SET(tcp_fd, &read_set);
        
        //wait for something to happen
        if (select(maxfd, &read_set, NULL, NULL, NULL) < 0) {
            log_printf(ERROR, "forward: Failed to wait for select in control loop: %s", strerror(errno));
        }

        // If we have something to read on UDP port, forward it to TCP
        if (FD_ISSET(udp_fd, &read_set)) {
            udp_to_tcp();
        }

        // If we have something to read on TCP port, forward it to UDP
        if (FD_ISSET(tcp_fd, &read_set)) {
            tcp_to_udp();
        }

    }
}

static void sigint_handler(int signum) {
    log_printf(DEBUG, "Stopping UDP/TCP converter");
    if (close(udp_fd) != 0) {
        log_printf(ERROR, "Failed to close udp_fd: %s", strerror(errno));
    }
    if (close(tcp_fd) != 0) {
        log_printf(ERROR, "Failed to close tcp_fd: %s", strerror(errno));
    }
    log_printf(DEBUG, "Finishing closing forwarding sockets");
}
    


int main () {
    logger_init(NET_HANDLER);
    signal(SIGINT, sigint_handler);
    
    start_udp_relay();
    
    start_tcp_exposed_port();
    
    forward(); // loops forever
    
    return 0;
}
    
