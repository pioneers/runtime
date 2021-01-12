#include "net_handler_client.h"

// throughout this code, net_handler is abbreviated "nh"
pid_t nh_pid;                           // holds the pid of the net_handler process
struct sockaddr_in udp_servaddr = {0};  // holds the udp server address
pthread_t dump_tid;                     // holds the thread id of the output dumper threads
pthread_mutex_t print_udp_mutex;        // lock over whether to print the next received udp
int print_next_udp;                     // 0 if we want to suppress incoming dev data, 1 to print incoming dev data to stdout

int nh_tcp_shep_fd = -1;      // holds file descriptor for TCP Shepherd socket
int nh_tcp_dawn_fd = -1;      // holds file descriptor for TCP Dawn socket
int nh_udp_fd = -1;           // holds file descriptor for UDP Dawn socket
FILE* default_tcp_fp = NULL;  // holds default output location of incoming TCP messages (stdout if NULL)
FILE* tcp_output_fp = NULL;   // holds current output location of incoming TCP messages
FILE* udp_output_fp = NULL;   // holds current output location of incoming UDP messages
FILE* null_fp = NULL;         // file pointer to /dev/null

// ************************************* HELPER FUNCTIONS ************************************** //

/**
 * Function to connect the net_handler_client to net_handler,
 * as the specified client.
 * Arguments:
 *    client: the client to connect as, one of DAWN or SHEPHERD
 * Returns: socket descriptor of new connection; exits on failure
 */
static int connect_tcp(robot_desc_field_t client) {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("socket: failed to create listening socket: %s\n", strerror(errno));
        stop_net_handler();
        exit(1);
    }

    int optval = 1;
    if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int))) != 0) {
        printf("setsockopt: failed to set listening socket for reuse of port: %s\n", strerror(errno));
    }

    // set the elements of cli_addr
    struct sockaddr_in cli_addr = {0};                                                   // initialize everything to 0
    cli_addr.sin_family = AF_INET;                                                       // use IPv4
    cli_addr.sin_port = (client == SHEPHERD) ? htons(SHEPHERD_PORT) : htons(DAWN_PORT);  // use specifid port to connect
    cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);                                        // use any address set on this machine to connect

    // bind the client side too, so that net_handler can verify it's the proper client
    if ((bind(sockfd, (struct sockaddr*) &cli_addr, sizeof(struct sockaddr_in))) != 0) {
        printf("bind: failed to bind listening socket to client port: %s\n", strerror(errno));
        close(sockfd);
        stop_net_handler();
        exit(1);
    }

    // set the elements of serv_addr
    struct sockaddr_in serv_addr = {0};          // initialize everything to 0
    serv_addr.sin_family = AF_INET;              // use IPv4
    serv_addr.sin_port = htons(RASPI_TCP_PORT);  // want to connect to raspi port
    serv_addr.sin_addr.s_addr = inet_addr(RASPI_ADDR);

    // connect to the server
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in)) != 0) {
        printf("connect: failed to connect to socket: %s\n", strerror(errno));
        close(sockfd);
        stop_net_handler();
        exit(1);
    }

    // send the verification byte
    uint8_t verif_byte = (client == SHEPHERD) ? 0 : 1;
    writen(sockfd, &verif_byte, 1);

    return sockfd;
}

/**
 * Function to receive data from net_handler on the UDP port
 * Arguments:
 *    udp_fd: file descriptor connected to UDP socket receiving data from net_handler
 */
static void recv_udp_data(int udp_fd) {
    int max_size = 4096;
    uint8_t msg[max_size];
    struct sockaddr_in recvaddr;
    socklen_t addrlen = sizeof(recvaddr);
    int recv_size;

    // receive message from udp socket
    if ((recv_size = recvfrom(udp_fd, msg, max_size, 0, (struct sockaddr*) &recvaddr, &addrlen)) < 0) {
        fprintf(udp_output_fp, "recvfrom: %s\n", strerror(errno));
    }

    // unpack the data
    DevData* dev_data = dev_data__unpack(NULL, recv_size, msg);
    if (dev_data == NULL) {
        printf("Error unpacking incoming message\n");
    }

    // display the message's fields.
    for (int i = 0; i < dev_data->n_devices; i++) {
        fprintf(udp_output_fp, "Device No. %d:", i);
        fprintf(udp_output_fp, "\ttype = %s, uid = %llu, itype = %d\n", dev_data->devices[i]->name, dev_data->devices[i]->uid, dev_data->devices[i]->type);
        fprintf(udp_output_fp, "\tParams:\n");
        for (int j = 0; j < dev_data->devices[i]->n_params; j++) {
            fprintf(udp_output_fp, "\t\tparam \"%s\" has type ", dev_data->devices[i]->params[j]->name);
            switch (dev_data->devices[i]->params[j]->val_case) {
                case (PARAM__VAL_FVAL):
                    fprintf(udp_output_fp, "FLOAT with value %f\n", dev_data->devices[i]->params[j]->fval);
                    break;
                case (PARAM__VAL_IVAL):
                    fprintf(udp_output_fp, "INT with value %d\n", dev_data->devices[i]->params[j]->ival);
                    break;
                case (PARAM__VAL_BVAL):
                    fprintf(udp_output_fp, "BOOL with value %s\n", dev_data->devices[i]->params[j]->bval ? "True" : "False");
                    break;
                default:
                    fprintf(udp_output_fp, "ERROR: no param value");
                    break;
            }
        }
    }

    // free the unpacked message
    dev_data__free_unpacked(dev_data, NULL);
    fflush(udp_output_fp);

    // if we were asked to print the last UDP message, set output back to /dev/null
    pthread_mutex_lock(&print_udp_mutex);
    if (print_next_udp) {
        print_next_udp = 0;
        udp_output_fp = null_fp;
    }
    pthread_mutex_unlock(&print_udp_mutex);
}

/**
 * Function to receive data as either Dawn or Shepherd on the connection
 * Arguments:
 *    client: client that we are receiving data as; one of SHEPHERD or DAWN
 *    tcp_fd: file descriptor connected to net_handler from which to read data
 * Returns: 0 on success, -1 on failure
 */
static int recv_tcp_data(robot_desc_field_t client, int tcp_fd) {
    // variables to read messages into
    Text* msg;
    net_msg_t msg_type;
    uint8_t* buf;
    uint16_t len;
    char client_str[16];
    if (client == SHEPHERD) {
        strcpy(client_str, "SHEPHERD");
    } else {
        strcpy(client_str, "DAWN");
    }

    // parse message
    if (parse_msg(tcp_fd, &msg_type, &len, &buf) == 0) {
        return -1;
    }

    // unpack the message
    if ((msg = text__unpack(NULL, len, buf)) == NULL) {
        fprintf(tcp_output_fp, "Error unpacking incoming message from %s\n", client_str);
    }

    // print the incoming message
    if (msg_type == LOG_MSG) {
        for (int i = 0; i < msg->n_payload; i++) {
            fprintf(tcp_output_fp, "%s", msg->payload[i]);
        }
    } else if (msg_type == CHALLENGE_DATA_MSG) {
        for (int i = 0; i < msg->n_payload; i++) {
            fprintf(tcp_output_fp, "Challenge %d result: %s\n", i, msg->payload[i]);
        }
    }
    fflush(tcp_output_fp);

    // free allocated memory
    free(buf);
    text__free_unpacked(msg, NULL);

    return 0;
}

/**
 * Dumps output from net_handler received on various ports to this process's
 * stdout in a human-readable way. Is meant to be run as a thread
 * Arguments:
 *    args: thread arugments; always NULL
 * Returns: NULL
 */
static void* output_dump(void* args) {
    const int sample_size = 20;              // number of messages that need to come in before disabling output
    const uint64_t disable_threshold = 50;   // if the interval between each of the past sample_size messages has been less than this many milliseconds, disable output
    const uint64_t enable_threshold = 1000;  // if this many milliseconds have passed between now and last received message, enable output
    uint64_t last_received_time = 0, curr_time;
    uint32_t less_than_disable_thresh = 0;

    // set up the read_set argument for select()
    fd_set read_set;
    int maxfd = (nh_tcp_dawn_fd > nh_tcp_shep_fd) ? nh_tcp_dawn_fd : nh_tcp_shep_fd;
    maxfd = (nh_udp_fd > maxfd) ? nh_udp_fd : maxfd;
    maxfd++;

    // wait for net handler to create some output, then print that output to stdout of this process
    while (1) {
        // set up the read_set argument to selct()
        FD_ZERO(&read_set);
        FD_SET(nh_tcp_shep_fd, &read_set);
        FD_SET(nh_tcp_dawn_fd, &read_set);
        FD_SET(nh_udp_fd, &read_set);

        // prepare to accept cancellation requests over the select
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        // wait for something to happen
        if (select(maxfd, &read_set, NULL, NULL, NULL) < 0) {
            printf("select: output dump: %s\n", strerror(errno));
        }

        // deny all cancellation requests until the next loop
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        // enable tcp output if more than enable_thresh has passed between last time and previous time
        if (FD_ISSET(nh_tcp_shep_fd, &read_set) || FD_ISSET(nh_tcp_dawn_fd, &read_set)) {
            curr_time = millis();
            if (curr_time - last_received_time >= enable_threshold) {
                less_than_disable_thresh = 0;
                if (default_tcp_fp == NULL) {
                    tcp_output_fp = stdout;
                } else {
                    tcp_output_fp = default_tcp_fp;
                }
            }
            if (curr_time - last_received_time <= disable_threshold) {
                less_than_disable_thresh++;
                if (less_than_disable_thresh == sample_size) {
                    printf("Suppressing output: too many messages...\n\n");
                    fflush(tcp_output_fp);
                    tcp_output_fp = null_fp;
                }
            }
            last_received_time = curr_time;
        }

        // print stuff from whichever file descriptors are ready for reading...
        if (FD_ISSET(nh_tcp_shep_fd, &read_set)) {
            if (recv_tcp_data(SHEPHERD, nh_tcp_shep_fd) == -1) {
                return NULL;
            }
        }
        if (FD_ISSET(nh_tcp_dawn_fd, &read_set)) {
            if (recv_tcp_data(DAWN, nh_tcp_dawn_fd) == -1) {
                return NULL;
            }
        }
        if (FD_ISSET(nh_udp_fd, &read_set)) {
            recv_udp_data(nh_udp_fd);
        }
    }
    return NULL;
}

// ************************************* NET HANDLER CLIENT FUNCTIONS ************************** //

void start_net_handler() {
    // fork net_handler process
    if ((nh_pid = fork()) < 0) {
        printf("fork: %s\n", strerror(errno));
    } else if (nh_pid == 0) {  // child
        // cd to the net_handler directory
        if (chdir("../net_handler") == -1) {
            printf("chdir: %s\n", strerror(errno));
        }

        // exec the actual net_handler process
        if (execlp("./net_handler", "net_handler", NULL) < 0) {
            printf("execlp: %s\n", strerror(errno));
        }

    } else {       // parent
        sleep(1);  // allows net_handler to set itself up

        // connect to the raspi networking ports to catch network output
        nh_tcp_dawn_fd = connect_tcp(DAWN);
        nh_tcp_shep_fd = connect_tcp(SHEPHERD);
        if ((nh_udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            printf("socket: UDP socket creation failed...\n");
            stop_net_handler();
            exit(1);
        }
        udp_servaddr.sin_family = AF_INET;
        udp_servaddr.sin_addr.s_addr = inet_addr(RASPI_ADDR);
        udp_servaddr.sin_port = htons(RASPI_UDP_PORT);

        // open /dev/null
        null_fp = fopen("/dev/null", "w");

        // init the mutex that will control whether udp prints to screen
        if (pthread_mutex_init(&print_udp_mutex, NULL) != 0) {
            printf("pthread_mutex_init: print udp mutex\n");
        }
        print_next_udp = 0;
        udp_output_fp = null_fp;  // by default set to output to /dev/null

        // start the thread that is dumping output from net_handler to stdout of this process
        if (pthread_create(&dump_tid, NULL, output_dump, NULL) != 0) {
            printf("pthread_create: output dump\n");
        }
        usleep(400000);  // allow time for thread to dump output before returning to client
        
        pthread_t thread_id; // id of thread running the keyboard_interface
        pthread_create(&thread_id, NULL, (void *)setup_keyboard, NULL);
    }
}

void stop_net_handler() {
    // send signal to net_handler and wait for termination
    if (kill(nh_pid, SIGINT) < 0) {
        printf("kill net handler: %s\n", strerror(errno));
    }
    if (waitpid(nh_pid, NULL, 0) < 0) {
        printf("waitpid net handler: %s\n", strerror(errno));
    }

    close_output();
}

void close_output() {
    // killing net handler should cause dump thread to return, so join with it
    if (pthread_join(dump_tid, NULL) != 0) {
        printf("pthread_join: output dump\n");
    }

    // close all the file descriptors
    if (nh_tcp_shep_fd != -1) {
        close(nh_tcp_shep_fd);
    }
    if (nh_tcp_dawn_fd != -1) {
        close(nh_tcp_dawn_fd);
    }
    if (nh_udp_fd != -1) {
        close(nh_udp_fd);
    }
}

void send_run_mode(robot_desc_field_t client, robot_desc_val_t mode) {
    RunMode run_mode = RUN_MODE__INIT;
    uint8_t* send_buf;
    uint16_t len;

    // set the right mode
    switch (mode) {
        case (IDLE):
            run_mode.mode = MODE__IDLE;
            break;
        case (AUTO):
            run_mode.mode = MODE__AUTO;
            break;
        case (TELEOP):
            run_mode.mode = MODE__TELEOP;
            break;
        default:
            printf("ERROR: sending run mode message\n");
    }

    // build the message
    len = run_mode__get_packed_size(&run_mode);
    send_buf = make_buf(RUN_MODE_MSG, len);
    run_mode__pack(&run_mode, send_buf + BUFFER_OFFSET);

    // send the message
    if (client == SHEPHERD) {
        writen(nh_tcp_shep_fd, send_buf, len + BUFFER_OFFSET);
    } else {
        writen(nh_tcp_dawn_fd, send_buf, len + BUFFER_OFFSET);
    }
    free(send_buf);
    usleep(400000);  // allow time for net handler and runtime to react and generate output before returning to client
}

void send_start_pos(robot_desc_field_t client, robot_desc_val_t pos) {
    StartPos start_pos = START_POS__INIT;
    uint8_t* send_buf;
    uint16_t len;

    // set the right mode
    switch (pos) {
        case (LEFT):
            start_pos.pos = POS__LEFT;
            break;
        case (RIGHT):
            start_pos.pos = POS__RIGHT;
            break;
        default:
            printf("ERROR: sending run mode message\n");
    }

    // build the message
    len = start_pos__get_packed_size(&start_pos);
    send_buf = make_buf(START_POS_MSG, len);
    start_pos__pack(&start_pos, send_buf + BUFFER_OFFSET);

    // send the message
    if (client == SHEPHERD) {
        writen(nh_tcp_shep_fd, send_buf, len + BUFFER_OFFSET);
    } else {
        writen(nh_tcp_dawn_fd, send_buf, len + BUFFER_OFFSET);
    }
    free(send_buf);
    usleep(400000);  // allow time for net handler and runtime to react and generate output before returning to client
}

void send_user_input(uint64_t buttons, float joystick_vals[4], robot_desc_field_t source) {
    UserInputs inputs = USER_INPUTS__INIT;

    // build the message
    inputs.n_inputs = 1;
    inputs.inputs = malloc(sizeof(Input*) * inputs.n_inputs);
    if (inputs.inputs == NULL) {
        printf("send_user_input: Failed to malloc inputs\n");
        exit(1);
    }
    Input input = INPUT__INIT;
    inputs.inputs[0] = &input;

    input.connected = 1;
    input.source = (source == GAMEPAD) ? SOURCE__GAMEPAD : SOURCE__KEYBOARD;
    input.buttons = buttons;
    input.n_axes = 4;
    input.axes = malloc(sizeof(double) * 4);
    if (input.axes == NULL) {
        printf("send_user_input: Failed to malloc axes\n");
        exit(1);
    }
    for (int i = 0; i < 4; i++) {
        input.axes[i] = joystick_vals[i];
    }

    uint16_t len = user_inputs__get_packed_size(&inputs);
    uint8_t* send_buf = malloc(len);
    if (send_buf == NULL) {
        printf("send_user_input: Failed to malloc buffer\n");
        exit(1);
    }
    user_inputs__pack(&inputs, send_buf);

    // send the message
    sendto(nh_udp_fd, send_buf, len, 0, (struct sockaddr*) &udp_servaddr, sizeof(struct sockaddr_in));

    // free everything
    free(input.axes);
    free(inputs.inputs);
    free(send_buf);
}

void send_challenge_data(robot_desc_field_t client, char** data, int num_challenges) {
    Text challenge_data = TEXT__INIT;
    uint8_t* send_buf;
    uint16_t len;

    // build the message
    challenge_data.payload = data;
    challenge_data.n_payload = num_challenges;
    len = text__get_packed_size(&challenge_data);
    send_buf = make_buf(CHALLENGE_DATA_MSG, len);
    text__pack(&challenge_data, send_buf + BUFFER_OFFSET);

    // send the message
    if (client == SHEPHERD) {
        writen(nh_tcp_shep_fd, send_buf, len + BUFFER_OFFSET);
    } else {
        writen(nh_tcp_dawn_fd, send_buf, len + BUFFER_OFFSET);
    }
    free(send_buf);
    usleep(400000);  // allow time for net handler and runtime to react and generate output before returning to client
}

void send_device_subs(dev_subs_t* subs, int num_devices) {
    DevData dev_data = DEV_DATA__INIT;
    uint8_t* send_buf;
    uint16_t len;

    // build the message
    device_t* curr_device;
    uint8_t curr_type;
    dev_data.n_devices = num_devices;
    dev_data.devices = malloc(sizeof(Device*) * num_devices);
    if (dev_data.devices == NULL) {
        printf("send_device_subs: Failed to malloc\n");
        exit(1);
    }

    // set each device
    for (int i = 0; i < num_devices; i++) {
        if ((curr_type = device_name_to_type(subs[i].name)) == (uint8_t) -1) {
            printf("ERROR: no such device \"%s\"\n", subs[i].name);
        }
        // fill in device fields
        curr_device = get_device(curr_type);
        Device* dev = malloc(sizeof(Device));
        if (dev == NULL) {
            printf("send_device_subs: Failed to malloc\n");
            exit(1);
        }
        device__init(dev);
        dev->name = curr_device->name;
        dev->uid = subs[i].uid;
        dev->type = curr_type;
        dev->n_params = curr_device->num_params;
        dev->params = malloc(sizeof(Param*) * curr_device->num_params);
        if (dev->params == NULL) {
            printf("send_device_subs: Failed to malloc\n");
            exit(1);
        }

        // set each param
        for (int j = 0; j < curr_device->num_params; j++) {
            // fill in param fields
            Param* prm = malloc(sizeof(Param));
            if (prm == NULL) {
                printf("send_device_subs: Failed to malloc\n");
                exit(1);
            }
            param__init(prm);
            prm->val_case = PARAM__VAL_BVAL;
            prm->bval = (subs[i].params & (1 << j)) ? 1 : 0;
            dev->params[j] = prm;
        }
        dev_data.devices[i] = dev;
    }
    len = dev_data__get_packed_size(&dev_data);
    send_buf = make_buf(DEVICE_DATA_MSG, len);
    dev_data__pack(&dev_data, send_buf + BUFFER_OFFSET);

    // send the message
    writen(nh_tcp_dawn_fd, send_buf, len + BUFFER_OFFSET);

    // free everything
    for (int i = 0; i < num_devices; i++) {
        for (int j = 0; j < dev_data.devices[i]->n_params; j++) {
            free(dev_data.devices[i]->params[j]);
        }
        free(dev_data.devices[i]->params);
        free(dev_data.devices[i]);
    }
    free(dev_data.devices);
    usleep(400000);  // allow time for net handler and runtime to react and generate output before returning to client
}

void print_next_dev_data() {
    pthread_mutex_lock(&print_udp_mutex);
    print_next_udp = 1;
    udp_output_fp = stdout;
    pthread_mutex_unlock(&print_udp_mutex);
    usleep(400000);  // allow time for output_dump to react and generate output before returning to client
}

void update_tcp_output_fp(char* output_address) {
    default_tcp_fp = fopen(output_address, "w");
}
