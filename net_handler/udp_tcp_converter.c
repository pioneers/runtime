/**
 * A process that:
 * 1) Intercepts incoming TCP messages to Runtime and sends it to net handler over UDP
 * 2) Intercepts outgoing UDP messages from net handler and sends it out over TCP
 * 
 * The motivation is that for Spring 2021, we're using ngrok to send data over the internet.
 * ngrok handles only TCP connections, so outgoing UDP messages must be converted to TCP.
 * Dawn will be doing the same.
 * The idea is that we can delete this process after Spring 2021 with no modifications to net handler.
 * 
 * TCP Messages will have the form:
 * [msg_type][packed_proto_length][packed_proto]
 * msg_type: 1 byte indicating the type of protobuf that is being transmitted
 * packed_proto_length: 2 bytes indicating the length of [packed_proto]
 * packed_proto: [packed_proto_length] bytes containing a packed protobuf
 * 
 * UDP Messages won't have the first two chunks of metadata, and will hav ejust
 * [packed_proto]
 * 
 * The converter reads TCP messages, strips the metadata, and sends just the packed proto over UDP to net handler
 * It reads packed protos from net handler over UDP, prepends it with [msg_type][p]cked_proto_lengtho, sends
 *   it over TCP to ngrok/Dawn
 * 
 * Visual Diagram:
 * Processes: [   RUNTIME    ] <---UDP---> [UDP_TCP_CONVERTER] <---TCP---> [ngrok] <---TCP---> [DAWN]
 * Ports:     [RASPI_UDP_PORT]             [N/A][FWD_TCP_PORT] 
 * The UDP port used by this process will be randomly set by the OS.
 * 
 * Messages normally sent between Runtime and Dawn over TCP is unaffected by this process.
 */

#include "../runtime_util/runtime_util.h"
#include "net_util.h"

#define FWD_TCP_PORT 1234              // port to expose with ngrok to Dawn. Must match ngrok.yml
struct sockaddr_in net_handler = {0};  // initialize sock address struct to net_handler

/**
 * Opens a UDP socket that is used to send messages to net handler.
 * Returns:
 *    the file descriptor for the UDP socket if successful
 *    -1 on failure
 */
int start_udp_relay() {
    int udp_fd = -1;
    if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        log_printf(ERROR, "start_udp_relay: could not create udp socket: %s", strerror(errno));
        return -1;
    }
    return udp_fd;
}

/**
 * Opens a TCP socket server to intercept incoming disguised UDP messages.
 * Returns:
 *    the file descriptor for the TCP server socket if successful
 *    -1 on failure
 */
int start_tcp_exposed_port() {
    //create socket
    int tcp_fd = -1;
    if ((tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        log_printf(ERROR, "start_tpc_exposed_port: failed to create listening socket: %s", strerror(errno));
        return -1;
    }

    //set the socket option SO_REUSEPORT on so that if raspi terminates and restarts it can immediately reopen the same port
    int optval = 1;
    if ((setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int))) != 0) {
        log_printf(ERROR, "start_tpc_exposed_port: failed to set listening socket for reuse of port: %s", strerror(errno));
    }

    //set the elements of serv_addr
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;                 //use IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //use any IP interface on raspi
    serv_addr.sin_port = htons(FWD_TCP_PORT);       //assign a port number

    //bind socket to well-known IP_addr:port
    if ((bind(tcp_fd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in))) != 0) {
        log_printf(ERROR, "start_tpc_exposed_port: failed to bind listening socket to raspi port: %s", strerror(errno));
        close(tcp_fd);
        return -1;
    }

    //set the socket to be in listening mode (since the robot is the server)
    if ((listen(tcp_fd, 2)) != 0) {
        log_printf(ERROR, "start_tpc_exposed_port: failed to set listening socket to listen mode: %s", strerror(errno));
        close(tcp_fd);
        return -1;
    }
    return tcp_fd;
}

/**
 * Reads a message from UDP and sends it over TCP.
 * Expects message type DeviceData.
 * 
 * Arguments:
 *    udp_fd: the file descriptor for the UDP socket to read from
 *    tcp_fd: the file descriptor for the TCP socket to send to
 * Returns:
 *    0 on susccess
 *    -1 on receive overflow
 *    -2 on some other receive error
 *    -3 on write error
 */
int udp_to_tcp(int udp_fd, int tcp_fd) {
    static int BUF_SIZE = 1024;
    uint8_t recv_buffer[BUF_SIZE];
    int recvlen;  // Since received UDP is just packed proto, this is also length of packed proto

    // read from udp and get packed proto with metadata
    recvlen = recvfrom(udp_fd, recv_buffer + BUFFER_OFFSET, BUF_SIZE, 0, NULL, NULL);
    if (recvlen == BUF_SIZE) {  // Message is very large; would overflow our buffer
        log_printf(WARN, "udp_to_tcp: UDP Read length matches read buffer size %d", recvlen);
        return -1;
    } else if (recvlen < 0) {  // Couldn't read message
        log_printf(ERROR, "udp_to_tcp: UDP recvfrom failed %s", strerror(errno));
        return -2;
    }

    // Prepend the received buffer with [msg_type][proto_length/recvlen]
    recv_buffer[0] = DEVICE_DATA_MSG;
    memcpy(&recv_buffer[1], (uint8_t*) &recvlen, 2);  // Copy the recvlen (2 bytes) into recv_buffer[0:1]

    // send data over tcp port
    size_t write_len = recvlen + BUFFER_OFFSET;
    if (writen(tcp_fd, recv_buffer, write_len) < 0) {
        log_printf(ERROR, "udp_to_tcp: writen error'd out: %s", strerror(errno));
        return -3;
    }

    return 0;
}

/**
 * Reads a message from TCP and sends it over UDP to Runtime.
 * Expects message type Input
 * 
 * Arguments:
 *   udp_fd: the file descriptor for the UDP socket to send to
 *   tcp_fd: the file descriptor for the TCP socket to read from
 * Returns:
 *   0 on success
 *   -1 on EOF (client disconnected)
 *   -2 on some other error
 */
int tcp_to_udp(int udp_fd, int tcp_fd) {
    net_msg_t msg_type;  // message type
    uint16_t len_pb;     // length of incoming serialized protobuf message
    uint8_t* buf;        // buffer to read packed proto into

    // Read and parse the next message from TCP
    int err = parse_msg(tcp_fd, &msg_type, &len_pb, &buf);
    if (err == 0) {  // Means there is EOF while reading which means client disconnected
        return -1;
    } else if (err == -1) {  // Means there is some other error while reading
        return -2;
    }

    // Send just the packed proto (no metadata) over on UDP to net handler
    err = sendto(udp_fd, buf, len_pb, 0, (const struct sockaddr*) &net_handler, sizeof(struct sockaddr));
    if (err < 0) {
        log_printf(ERROR, "tcp_to_udp: UDP sendto failed: %s", strerror(errno));
        return -2;
    } else if (err != len_pb) {
        log_printf(WARN, "tcp_to_udp: Didn't send all data to Runtime. send buffer length %d, actual sent %d", len_pb, err);
        return -2;
    }
    free(buf);
    return 0;
}

/**
 * Main function.
 * Reads for messages from one protocol and sends it over the other:
 * Reads from udp_fd writes to tcp_fd
 * Reads from tcp_fd writes to udp_fd
 * Arguments:
 *    udp_fd: file descriptor of UDP socket
 *    tcp_fd: file descriptor for TCP socket
 */
void forward(int udp_fd, int tcp_fd) {
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
            udp_to_tcp(udp_fd, tcp_fd);
        }

        // If we have something to read on TCP port, forward it to UDP
        if (FD_ISSET(tcp_fd, &read_set)) {
            tcp_to_udp(udp_fd, tcp_fd);
        }
    }
}

// ********************************** MAIN ********************************** //

/**
 * Sends a UDP message to Runtime so that Runtime will
 * send all outgoing UDP messages to this process.
 * 
 * This sends an empty input state.
 */
int send_gamepad_disconnected(int udp_fd) {
    UserInputs inputs = USER_INPUTS__INIT;

    // build the message
    // We will send one Gamepad input signalling that it is disconnected
    inputs.n_inputs = 1;
    inputs.inputs = malloc(sizeof(Input*) * inputs.n_inputs);
    if (inputs.inputs == NULL) {
        log_printf(ERROR, "send_gamepad_disconnected: Failed to malloc inputs\n");
        exit(1);
    }
    Input input = INPUT__INIT;
    inputs.inputs[0] = &input;

    // Disconnected input
    input.connected = 0;  // Disconnect gamepad signal
    input.source = SOURCE__GAMEPAD;
    input.buttons = 0;  // No buttons pressed
    input.n_axes = 4;
    input.axes = malloc(sizeof(double) * 4);
    if (input.axes == NULL) {
        log_printf(FATAL, "send_gamepad_disconnected: Failed to malloc axes\n");
        exit(1);
    }
    for (int i = 0; i < 4; i++) {
        input.axes[i] = 0;  // Neutral joysticks
    }

    uint16_t len = user_inputs__get_packed_size(&inputs);
    uint8_t* send_buf = malloc(len);
    if (send_buf == NULL) {
        log_printf(FATAL, "send_gamepad_disconnected: Failed to malloc buffer\n");
        exit(1);
    }
    user_inputs__pack(&inputs, send_buf);

    // send the message
    sendto(udp_fd, send_buf, len, 0, (struct sockaddr*) &net_handler, sizeof(struct sockaddr_in));

    // free everything
    free(input.axes);
    free(inputs.inputs);
    free(send_buf);
}

int main() {
    logger_init(NET_HANDLER);

    int udp_fd = start_udp_relay();

    // Start TCP server, then accept a TCP client
    int tcp_server_fd = start_tcp_exposed_port();
    int tcp_fd = accept(tcp_server_fd, NULL, NULL);  // File descriptor to talk with client

    // Initiate UDP connection with Runtime
    net_handler.sin_family = AF_INET;
    net_handler.sin_addr.s_addr = inet_addr(RASPI_ADDR);
    net_handler.sin_port = htons(RASPI_UDP_PORT);

    // Send a one-time UDP message to Runtime
    send_gamepad_disconnected(udp_fd);

    // Execute main process
    forward(udp_fd, tcp_fd);  // loops forever

    return 0;
}
