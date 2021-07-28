#include "net_handler_client.h"

// throughout this code, net_handler is abbreviated "nh"
pid_t nh_pid;                           // holds the pid of the net_handler process
struct sockaddr_in udp_servaddr = {0};  // holds the udp server address
pthread_t dump_tid;                     // holds the thread id of the output dumper threads

int nh_tcp_shep_fd = -1;      // holds file descriptor for TCP Shepherd socket
int nh_tcp_dawn_fd = -1;      // holds file descriptor for TCP Dawn socket
int nh_udp_fd = -1;           // holds file descriptor for UDP Dawn socket
FILE* default_tcp_fp = NULL;  // holds default output location of incoming TCP messages (stdout if NULL)
FILE* tcp_output_fp = NULL;   // holds current output location of incoming TCP messages
FILE* null_fp = NULL;         // file pointer to /dev/null

DevData* device_data = NULL;       // Holds the most recent device_data received from Runtime
pthread_mutex_t device_data_lock;  // Mutual exclusion lock on the device_data for multithreaded access

// 2021 Game Specific
bool hypothermia_enabled = false;  // 0 if hypothermia enabled, 1 if disabled

// ************************************* HELPER FUNCTIONS ************************************** //

/**
 * Function to connect the net_handler_client to net_handler,
 * as the specified client.
 * Arguments:
 *    client: the client to connect as, one of DAWN or SHEPHERD
 * Returns: socket descriptor of new connection; kills net handler and exits on failure
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
    if (writen(sockfd, &verif_byte, 1) == -1) {
        printf("writen: error sending verification byte\n");
        close(sockfd);
        stop_net_handler();
        exit(1);
    }

    return sockfd;
}

/**
 * Receives device data from the net_handler on the UDP fd and updates the corresponding global variable
 *
 * Args:
 *    udp_fd: file descriptor connected to UDP socket receiving data from net_handler
 * Returns:
 *    0 on success and -1 on error
 */
static int recv_udp_data(int udp_fd) {
    static int max_size = 4096;
    uint8_t msg[max_size];
    int recv_size;

    // receive message from udp socket
    if ((recv_size = recvfrom(udp_fd, msg, max_size, 0, NULL, NULL)) < 0) {
        printf("recv_udp_data: Failed to receive from UDP: %s\n", strerror(errno));
        return -1;
    }

    pthread_mutex_lock(&device_data_lock);
    // Must free old device data
    if (device_data != NULL) {
        dev_data__free_unpacked(device_data, NULL);
    }

    // unpack the new data
    device_data = dev_data__unpack(NULL, recv_size, msg);
    pthread_mutex_unlock(&device_data_lock);

    if (device_data == NULL) {
        printf("recv_udp_data: Error unpacking DevData message\n");
        return -1;
    }

    return 0;
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

    if (msg_type == TIME_STAMP_MSG) {
        TimeStamps* time_stamp_msg = time_stamps__unpack(NULL, len, buf);
        if (time_stamp_msg == NULL) {
            fprintf(tcp_output_fp, "Cannot unpack time_stamp msg");
        }
        uint64_t final_timestamp = millis();
        printf("First Dawn Timestamp: %llu ms\n", time_stamp_msg->dawn_timestamp);
        printf("Runtime Timestamp: %llu ms\n", time_stamp_msg->runtime_timestamp);
        printf("Final Dawn Timestamp: %llu ms\n", final_timestamp);
        printf("Round Dawn trip: %llu ms\n", final_timestamp - time_stamp_msg->dawn_timestamp);
        time_stamps__free_unpacked(time_stamp_msg, NULL);
    } else if (msg_type == LOG_MSG) {
        if ((msg = text__unpack(NULL, len, buf)) == NULL) {
            fprintf(tcp_output_fp, "Error unpacking incoming message from %s\n", client_str);
        }
        // unpack the message
        for (int i = 0; i < msg->n_payload; i++) {
            fprintf(tcp_output_fp, "%s", msg->payload[i]);
        }
        fflush(tcp_output_fp);
        text__free_unpacked(msg, NULL);
    } else {
        fprintf(tcp_output_fp, "Invalid message received over tcp from %s\n", client_str);
    }

    // free allocated memory
    free(buf);

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
    while (true) {
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
            if (recv_udp_data(nh_udp_fd) == -1) {
                return NULL;
            }
        }
    }
    return NULL;
}

// ************************************* NET HANDLER CLIENT FUNCTIONS ************************** //

void connect_clients(bool dawn, bool shepherd) {
    // Connect Dawn and Shepherd to net handler over TCP
    nh_tcp_dawn_fd = (dawn) ? connect_tcp(DAWN) : -1;
    nh_tcp_shep_fd = (shepherd) ? connect_tcp(SHEPHERD) : -1;

    // Open a single UDP socket to connect to net handler
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

    // init the mutex that will control access to the global device data variable
    if (pthread_mutex_init(&device_data_lock, NULL) != 0) {
        printf("pthread_mutex_init: print udp mutex\n");
    }

    // start the thread that is dumping output from net_handler to stdout of this process
    if (pthread_create(&dump_tid, NULL, output_dump, NULL) != 0) {
        printf("pthread_create: output dump\n");
    }
}

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

    } else {                    // parent
        sleep(1);               // allows net_handler to set itself up
        connect_clients(1, 1);  // Connect both fake Dawn and fake Shepherd
        usleep(400000);         // allow time for thread to dump output before returning to client
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
        if (writen(nh_tcp_shep_fd, send_buf, len + BUFFER_OFFSET) == -1) {
            printf("writen: issue sending run mode message\n");
        }
    } else {
        if (writen(nh_tcp_dawn_fd, send_buf, len + BUFFER_OFFSET) == -1) {
            printf("writen: issue sending run mode message\n");
        }
    }
    free(send_buf);
    usleep(400000);  // allow time for net handler and runtime to react and generate output before returning to client
}

void send_game_state(robot_desc_field_t state) {
    GameState game_state = GAME_STATE__INIT;
    uint8_t* send_buf;
    uint16_t len;

    switch (state) {
        case (POISON_IVY):
            game_state.state = STATE__POISON_IVY;
            break;
        case (DEHYDRATION):
            game_state.state = STATE__DEHYDRATION;
            break;
        case (HYPOTHERMIA):
            if (hypothermia_enabled) {
                game_state.state = STATE__HYPOTHERMIA_END;
                hypothermia_enabled = false;
                break;
            } else {
                game_state.state = STATE__HYPOTHERMIA_START;
                hypothermia_enabled = true;
                break;
            }
        default:
            printf("ERROR: sending game state message\n");
    }
    len = game_state__get_packed_size(&game_state);
    send_buf = make_buf(GAME_STATE_MSG, len);
    game_state__pack(&game_state, send_buf + BUFFER_OFFSET);

    // send the message
    if (writen(nh_tcp_shep_fd, send_buf, len + BUFFER_OFFSET) == -1) {
        printf("writen: issue sending game state message\n");
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
        if (writen(nh_tcp_shep_fd, send_buf, len + BUFFER_OFFSET) == -1) {
            printf("writen: issue sending start position message to shepherd\n");
        }
    } else {
        writen(nh_tcp_dawn_fd, send_buf, len + BUFFER_OFFSET);
        printf("writen: issue sending start position message to dawn\n");
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
    if (writen(nh_tcp_dawn_fd, send_buf, len + BUFFER_OFFSET) == -1) {
        printf("writen: issue sending device subs message to shepherd\n");
    }

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

DevData* get_next_dev_data() {
    pthread_mutex_lock(&device_data_lock);
    // We need to copy the data in the device_data Protobuf to return to the caller
    // The easiest way I found was to pack it into a buffer then unpack it again, but this may not be optimal
    int pb_size = dev_data__get_packed_size(device_data);
    uint8_t* buffer = malloc(pb_size);
    dev_data__pack(device_data, buffer);
    DevData* device_copy = dev_data__unpack(NULL, pb_size, buffer);
    pthread_mutex_unlock(&device_data_lock);
    free(buffer);
    return device_copy;  // must be freed by caller with dev_data__free_unpacked
}

void send_timestamp() {
    TimeStamps timestamp_msg = TIME_STAMPS__INIT;
    uint8_t* send_buf;
    uint16_t len;
    timestamp_msg.dawn_timestamp = millis();
    len = time_stamps__get_packed_size(&timestamp_msg);
    send_buf = make_buf(TIME_STAMP_MSG, len);
    time_stamps__pack(&timestamp_msg, send_buf + BUFFER_OFFSET);
    if (writen(nh_tcp_dawn_fd, send_buf, len + BUFFER_OFFSET) == -1) {
        printf("writen: issue sending timestamp to Dawn\n");
    }
    free(send_buf);
}

void update_tcp_output_fp(char* output_address) {
    default_tcp_fp = fopen(output_address, "w");
}
