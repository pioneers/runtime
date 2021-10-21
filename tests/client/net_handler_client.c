#include "net_handler_client.h"

// throughout this code, net_handler is abbreviated "nh"
pid_t nh_pid = -1;   // holds the pid of the net_handler process
pthread_t dump_tid;  // holds the thread id of the output dumper threads

// File pointers
int nh_tcp_shep_fd = -1;     // holds file descriptor for TCP Shepherd socket
int nh_tcp_dawn_fd = -1;     // holds file descriptor for TCP Dawn socket
FILE* tcp_output_fp = NULL;  // holds current output location of incoming TCP messages
FILE* null_fp = NULL;        // file pointer to /dev/null

/**
 * A variable holds the most recent device data received from Runtime
 * - This is updated constantly, and done while holding the lock
 * - Before updating this variable, remember to free it since it holds the previous message
 *   which has memory allocated to it. This prevents memory leak.
 * The mutex must be held whenever we want to access this global variable.
 */
pthread_mutex_t most_recent_dev_data_mutex;  // lock over the following two variables
DevData* most_recent_dev_data = NULL;        // Holds the most recent device_data received from Runtime

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
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket: failed to create listening socket: %s\n", strerror(errno));
        stop_net_handler();
        exit(1);
    }

    int optval = 1;
    if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int))) != 0) {
        printf("setsockopt: failed to set listening socket for reuse of port: %s\n", strerror(errno));
    }

    // set the elements of serv_addr
    struct sockaddr_in serv_addr = {0};  // initialize everything to 0
    serv_addr.sin_family = AF_INET;      // use IPv4
    serv_addr.sin_port = htons(6000);    // want to connect to raspi port
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

void print_dev_data(FILE* file, DevData* dev_data) {
    // display the message's fields.
    printf("About to print\n");
    for (int i = 0; i < dev_data->n_devices; i++) {
        fprintf(file, "Device No. %d:", i);
        fprintf(file, "\ttype = %s, uid = %llu, itype = %d\n", dev_data->devices[i]->name, dev_data->devices[i]->uid, dev_data->devices[i]->type);
        fprintf(file, "\tParams:\n");
        for (int j = 0; j < dev_data->devices[i]->n_params; j++) {
            fprintf(file, "\t\tparam \"%s\" has type ", dev_data->devices[i]->params[j]->name);
            switch (dev_data->devices[i]->params[j]->val_case) {
                case (PARAM__VAL_FVAL):
                    fprintf(file, "FLOAT with value %f\n", dev_data->devices[i]->params[j]->fval);
                    break;
                case (PARAM__VAL_IVAL):
                    fprintf(file, "INT with value %d\n", dev_data->devices[i]->params[j]->ival);
                    break;
                case (PARAM__VAL_BVAL):
                    fprintf(file, "BOOL with value %s\n", dev_data->devices[i]->params[j]->bval ? "True" : "False");
                    break;
                default:
                    fprintf(file, "ERROR: no param value");
                    break;
            }
        }
    }
}

/**
 * Function to receive data as either Dawn or Shepherd on the connection.
 * Receives the next message on the TCP socket and prints out the contents.
 * Arguments:
 *    client: client that we are receiving data as; one of SHEPHERD or DAWN
 *    tcp_fd: file descriptor connected to net_handler from which to read data
 * Returns:
 *    the type of message successfully received and printed out, or
 *    -1 on failure
 */
static int recv_tcp_data(robot_desc_field_t client, int tcp_fd) {
    // variables to read messages into
    char client_str[16];
    if (client == SHEPHERD) {
        strcpy(client_str, "SHEPHERD");
    } else {
        strcpy(client_str, "DAWN");
    }

    // parse message
    net_msg_t msg_type;
    uint8_t* buf;
    uint16_t len;
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
        Text* msg = text__unpack(NULL, len, buf);
        if (msg == NULL) {
            fprintf(tcp_output_fp, "Error unpacking incoming message from %s\n", client_str);
        }
        // unpack the message
        for (int i = 0; i < msg->n_payload; i++) {
            fprintf(tcp_output_fp, "%s", msg->payload[i]);
        }
        fflush(tcp_output_fp);
        text__free_unpacked(msg, NULL);
    } else if (msg_type == DEVICE_DATA_MSG) {
        // Update the global variable containing the most recent device data
        pthread_mutex_lock(&most_recent_dev_data_mutex);
        // Free the previous most recent device data
        if (most_recent_dev_data != NULL) {
            dev_data__free_unpacked(most_recent_dev_data, NULL);
        }
        most_recent_dev_data = dev_data__unpack(NULL, len, buf);
        if (most_recent_dev_data == NULL) {
            fprintf(tcp_output_fp, "Error unpacking incoming message from %s\n", client_str);
        }
        pthread_mutex_unlock(&most_recent_dev_data_mutex);
    } else {
        fprintf(tcp_output_fp, "Invalid message received over tcp from %s\n", client_str);
        msg_type = -1;  // Set the return value as -1 to indicate failure
    }

    // free allocated memory
    free(buf);
    return msg_type;
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
    maxfd++;

    // Initialize the output file to stdout
    if (tcp_output_fp == NULL) {
        tcp_output_fp = stdout;
    }

    // wait for net handler to create some output, then print that output to stdout of this process
    while (true) {
        // set up the read_set argument to select()
        FD_ZERO(&read_set);
        FD_SET(nh_tcp_shep_fd, &read_set);
        FD_SET(nh_tcp_dawn_fd, &read_set);

        // prepare to accept cancellation requests over the select
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        // wait for something to happen
        if (select(maxfd, &read_set, NULL, NULL, NULL) < 0) {
            printf("select: output dump: %s\n", strerror(errno));
        }

        // deny all cancellation requests until the next loop
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        // print stuff from whichever file descriptors are ready for reading...
        int msg_type = -1;
        if (FD_ISSET(nh_tcp_shep_fd, &read_set)) {
            if ((msg_type = recv_tcp_data(SHEPHERD, nh_tcp_shep_fd)) == -1) {
                return NULL;
            }
        }
        if (FD_ISSET(nh_tcp_dawn_fd, &read_set)) {
            if ((msg_type = recv_tcp_data(DAWN, nh_tcp_dawn_fd)) == -1) {
                return NULL;
            }
        }

        // enable tcp output if more than enable_thresh has passed between last time and previous time
        // It's expected to be spammed with Device Data messages, so we do this logic for only other message types
        if (msg_type != DEVICE_DATA_MSG) {
            if (FD_ISSET(nh_tcp_shep_fd, &read_set) || FD_ISSET(nh_tcp_dawn_fd, &read_set)) {
                curr_time = millis();
                if (curr_time - last_received_time >= enable_threshold) {  // Start printing output again
                    less_than_disable_thresh = 0;
                    tcp_output_fp = stdout;
                }
                if (curr_time - last_received_time <= disable_threshold) {  // Too many messages; Set output to /dev/null
                    less_than_disable_thresh++;
                    if (less_than_disable_thresh == sample_size) {
                        printf("Suppressing output: too many messages...\n\n");
                        fflush(tcp_output_fp);
                        tcp_output_fp = null_fp;
                    }
                }
                last_received_time = curr_time;
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

    // open /dev/null, which will be used to disable log output if there are too many logs
    null_fp = fopen("/dev/null", "w");

    // init the mutex that will control whether device data messages should be printed to screen
    if (pthread_mutex_init(&most_recent_dev_data_mutex, NULL) != 0) {
        printf("pthread_mutex_init: print device data mutex\n");
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
    // send signal to net_handler and wait for terminationZ
    if (nh_pid != -1) {
        if (kill(nh_pid, SIGINT) < 0) {
            printf("kill net handler: %s\n", strerror(errno));
        }
        if (waitpid(nh_pid, NULL, 0) < 0) {
            printf("waitpid net handler: %s\n", strerror(errno));
        }
    }

    // Need to cancel thread due to while loop, then join to ensure thread is finished
    if (pthread_cancel(dump_tid) != 0) {
        printf("pthread_cancel: canceling output_dump failed: %s\n", strerror(errno));
    }
    if (pthread_join(dump_tid, NULL) != 0) {
        printf("pthread_join: joining on output_dump failed: %s\n", strerror(errno));
    }

    // close all the file descriptors
    if (nh_tcp_shep_fd != -1) {
        close(nh_tcp_shep_fd);
    }
    if (nh_tcp_dawn_fd != -1) {
        close(nh_tcp_dawn_fd);
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
            printf("writen: issue sending start position message from shepherd\n");
        }
    } else if (writen(nh_tcp_dawn_fd, send_buf, len + BUFFER_OFFSET) == -1) {
        printf("writen: issue sending start position message from dawn\n");
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
    uint8_t* send_buf = make_buf(INPUTS_MSG, len);
    if (send_buf == NULL) {
        printf("send_user_input: Failed to malloc buffer\n");
        exit(1);
    }
    user_inputs__pack(&inputs, send_buf + BUFFER_OFFSET);

    // send the message
    if (writen(nh_tcp_dawn_fd, send_buf, len + BUFFER_OFFSET) == -1) {
        printf("send_user_input: Error when sending UserInput message");
        exit(1);
    }

    // free everything
    free(input.axes);
    free(inputs.inputs);
    free(send_buf);
}

void disconnect_user_input() {
    UserInputs inputs = USER_INPUTS__INIT;

    // build the message
    inputs.n_inputs = 2;
    inputs.inputs = malloc(sizeof(Input*) * inputs.n_inputs);
    if (inputs.inputs == NULL) {
        printf("disconnect_user_input: Failed to malloc inputs\n");
        exit(1);
    }

    // Disconnect Gamepad
    Input gamepad = INPUT__INIT;
    inputs.inputs[0] = &gamepad;
    gamepad.connected = 0;
    gamepad.source = SOURCE__GAMEPAD;
    gamepad.buttons = 0;
    gamepad.n_axes = 4;
    gamepad.axes = malloc(sizeof(double) * gamepad.n_axes);
    if (gamepad.axes == NULL) {
        printf("disconnect_user_input: Failed to malloc axes\n");
        exit(1);
    }
    for (int i = 0; i < gamepad.n_axes; i++) {
        gamepad.axes[i] = 0;
    }

    // Disconnect Keyboard
    Input keyboard = INPUT__INIT;
    inputs.inputs[1] = &keyboard;
    keyboard.connected = 0;
    keyboard.source = SOURCE__KEYBOARD;
    keyboard.buttons = 0;
    keyboard.n_axes = 0;

    uint16_t len = user_inputs__get_packed_size(&inputs);
    uint8_t* send_buf = make_buf(INPUTS_MSG, len);
    if (send_buf == NULL) {
        printf("disconnect_user_input: Failed to malloc buffer\n");
        exit(1);
    }
    user_inputs__pack(&inputs, send_buf + BUFFER_OFFSET);

    // send the message
    if (writen(nh_tcp_dawn_fd, send_buf, len + BUFFER_OFFSET) == -1) {
        printf("disconnect_user_input: Error when sending UserInput message");
        exit(1);
    }

    // free everything
    free(gamepad.axes);
    free(inputs.inputs);
    free(send_buf);
}

DevData* get_next_dev_data() {
    pthread_mutex_lock(&most_recent_dev_data_mutex);
    // We need to copy the data in the device_data Protobuf to return to the caller
    // The easiest way I (Ashwin) found was to pack it into a buffer then unpack it again, but this may not be optimal
    if (most_recent_dev_data == NULL) {
        return NULL;
    }
    int pb_size = dev_data__get_packed_size(most_recent_dev_data);
    uint8_t* buffer = malloc(pb_size);
    dev_data__pack(most_recent_dev_data, buffer);
    DevData* device_copy = dev_data__unpack(NULL, pb_size, buffer);
    pthread_mutex_unlock(&most_recent_dev_data_mutex);
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
