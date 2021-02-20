/**
 * A test process for udp_tcp_converter.c
 * This process receives messages from Runtime traditionally over UDP (Device Data), but over TCP.
 * This prints out the contents of received Device Data messages, and sends out random gamepad inputs
 * 
 * Visual Diagram:
 * Processes: [   RUNTIME    ] <---UDP---> [UDP_TCP_CONVERTER] <---TCP---> [udp_tcp_converter_test]
 * Ports:     [RASPI_UDP_PORT]             [N/A][FWD_TCP_PORT] 
 */
#include "../runtime_util/runtime_util.h"
#include "net_util.h"

#define FWD_TCP_PORT 1234  // This should be the same as in udp_tcp_converter.c

/**
 * Connects to the UDP_TCP_CONVERTER as a TCP client.
 * 
 * Returns the file descriptor for the TCP connection socket.
 */
int start_tcp_exposed_port() {
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

    //set the elements of serv_addr (Address of UDP_TCP_CONVERTER)
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;                      //use IPv4
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  //TODO: use same IP interface as the converter on raspi
    serv_addr.sin_port = htons(FWD_TCP_PORT);            //The port that UDP_TCP_CONVERTER listens on

    //bind socket to well-known IP_addr:port
    if (connect(tcp_fd, (const struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in)) != 0) {
        log_printf(ERROR, "start_tcp_exposed_port: failed to connect to server: %s", strerror(errno));
    }

    return tcp_fd;
}

/*
 * Receives new message from client on TCP connection and processes the message.
 * Arguments:
 *    - int conn_fd: socket connection's file descriptor from which to read the message
 *    - int print_dev_data: boolean of whether we should print the contents of the incoming Device Data
 * Returns: pointer to integer in which return status will be stored
 *      0 if message received and processed
 *     -1 if message could not be parsed because client disconnected and connection closed
 *     -2 if message could not be unpacked or other error
 */
static int recv_new_msg(int conn_fd, int print_dev_data) {
    net_msg_t msg_type;  //message type
    uint16_t len_pb;     //length of incoming serialized protobuf message
    uint8_t* buf;        //buffer to read raw data into

    int err = parse_msg(conn_fd, &msg_type, &len_pb, &buf);
    if (err == 0) {  // Means there is EOF while reading which means client disconnected
        return -1;
    } else if (err == -1) {  // Means there is some other error while reading
        return -2;
    }

    if (msg_type == DEVICE_DATA_MSG && print_dev_data) {  // Print out device data
        DevData* dev_data_msg = dev_data__unpack(NULL, len_pb, buf);
        if (dev_data_msg == NULL) {
            log_printf(ERROR, "recv_new_msg: Cannot unpack dev_data msg");
            return -2;
        }
        // For each device, place print out each param's values
        char* param_name;
        int param_idx;
        device_t* device;
        param_type_t param_type;
        for (int i = 0; i < dev_data_msg->n_devices; i++) {
            Device* device_proto = dev_data_msg->devices[i];
            device = get_device(device_proto->type);
            if (device == NULL) {
                log_printf(DEBUG, "Couldn't get device for type %d", device_proto->type);
            }
            for (int j = 0; j < device_proto->n_params; j++) {
                param_name = device_proto->params[j]->name;
                param_idx = get_param_idx(device->type, param_name);
                param_type = device->params[param_idx].type;
                switch (param_type) {
                    case INT:
                        log_printf(INFO, "%s: %d", param_name, device_proto->params[j]->ival);
                        break;
                    case FLOAT:
                        log_printf(INFO, "%s: %f", param_name, device_proto->params[j]->fval);
                        break;
                    case BOOL:
                        log_printf(INFO, "%s: %s", param_name, device_proto->params[j]->bval ? "TRUE" : "FALSE");
                        break;
                }
            }
        }
        dev_data__free_unpacked(dev_data_msg, NULL);
    }
    if (msg_type != DEVICE_DATA_MSG) {
        log_printf(WARN, "recv_new_msg: Received something other than DeviceData: %d", msg_type);
    }
    free(buf);
    return 0;
}

/**
 * Sends an arbitrary gamepad input
 */
int send_updated_gamepad(int tcp_fd) {
    UserInputs inputs = USER_INPUTS__INIT;

    // build the message
    // We will send one Gamepad input signalling that it is disconnected
    inputs.n_inputs = 1;
    inputs.inputs = malloc(sizeof(Input*) * inputs.n_inputs);
    if (inputs.inputs == NULL) {
        log_printf(FATAL, "send_gamepad_disconnected: Failed to malloc inputs\n");
        exit(1);
    }
    Input input = INPUT__INIT;
    inputs.inputs[0] = &input;

    //
    input.connected = 1;
    input.source = SOURCE__GAMEPAD;
    input.buttons = 1;  // Just 1
    input.n_axes = 4;
    input.axes = malloc(sizeof(double) * 4);
    if (input.axes == NULL) {
        log_printf(FATAL, "send_gamepad_disconnected: Failed to malloc axes\n");
        exit(1);
    }
    for (int i = 0; i < 4; i++) {
        input.axes[i] = 1.0;  // All joysticks 1.0
    }

    // Pack input protobuf into a buffer with TCP metadata (something we usually don't do for input protos)
    uint16_t len = user_inputs__get_packed_size(&inputs);
    uint8_t* send_buf = make_buf(INPUTS_MSG, len);
    if (send_buf == NULL) {
        log_printf(FATAL, "send_gamepad_disconnected: Failed to malloc buffer\n");
        exit(1);
    }
    user_inputs__pack(&inputs, send_buf + BUFFER_OFFSET);

    // send the message
    size_t write_len = len + BUFFER_OFFSET;
    if (writen(tcp_fd, send_buf, write_len) < 0) {
        log_printf(ERROR, "udp_to_tcp: writen error'd out: %s", strerror(errno));
    }

    // free everything
    free(input.axes);
    free(inputs.inputs);
    free(send_buf);
}

// ********************************** MAIN ********************************** //

int main() {
    logger_init(TEST);

    // Get tcp fd to communicate with UDP_TCP_CONVERTER
    int tcp_fd = start_tcp_exposed_port();

    int i = 0;
    while (1) {
        // Print only every so often
        if (i % 20 == 0) {
            log_printf(DEBUG, "Going to print incoming device data");
            recv_new_msg(tcp_fd, 1);
            log_printf(DEBUG, "Going to send a gamepad input");
            send_updated_gamepad(tcp_fd);
        } else {
            recv_new_msg(tcp_fd, 0);
        }
        i++;
    }
}
