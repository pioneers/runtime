#include "tcp_conn.h"

//used for creating and cleaning up TCP connection
typedef struct {
    int conn_fd;
    int challenge_fd;
    int send_logs;
    FILE* log_file;
    robot_desc_field_t client;
} tcp_conn_args_t;

pthread_t dawn_tid, shepherd_tid;

/*
 * Clean up memory and file descriptors before exiting from tcp_process
 * Arguments:
 *    - void *args: should be a pointer to tcp_conn_args_t populated with the listed descriptors and pointers
 */
static void tcp_conn_cleanup(void* args) {
    tcp_conn_args_t* tcp_args = (tcp_conn_args_t*) args;
    robot_desc_write(RUN_MODE, IDLE);

    if (close(tcp_args->conn_fd) != 0) {
        log_printf(ERROR, "Failed to close conn_fd: %s", strerror(errno));
    }
    if (close(tcp_args->challenge_fd) != 0) {
        log_printf(ERROR, "Failed to close challenge_fd: %s", strerror(errno));
    }
    if (tcp_args->log_file != NULL) {
        if (fclose(tcp_args->log_file) != 0) {
            log_printf(ERROR, "Failed to close log_file: %s", strerror(errno));
        }
        tcp_args->log_file = NULL;
    }
    robot_desc_write(tcp_args->client, DISCONNECTED);
    if (tcp_args->client == DAWN) {
        // Disconnect inputs if Dawn is no longer connected
        robot_desc_write(GAMEPAD, DISCONNECTED);
        robot_desc_write(KEYBOARD, DISCONNECTED);
    }
    free(args);
}


/*
 * Send a log message on the TCP connection to the client. Reads lines from the pipe until there is no more data
 * or it has read MAX_NUM_LOGS lines from the pipe, packages the message, and sends it.
 * Arguments:
 *    - int conn_fd: socket connection's file descriptor on which to write to the TCP port
 */
static void send_log_msg(int conn_fd, FILE* log_file) {
    char nextline[MAX_LOG_LEN];  //next log line read from FIFO pipe
    Text log_msg = TEXT__INIT;   //initialize a new Text protobuf message
    log_msg.n_payload = 0;       // The number of logs in this payload
    log_msg.payload = malloc(MAX_NUM_LOGS * sizeof(char*));
    if (log_msg.payload == NULL) {
        log_printf(FATAL, "send_log_msg: Failed to malloc payload for logs");
        exit(1);
    }

    //read in log lines until there are no more to read, or we read MAX_NUM_LOGS lines
    while (log_msg.n_payload < MAX_NUM_LOGS) {
        if (fgets(nextline, MAX_LOG_LEN, log_file) != NULL) {
            log_msg.payload[log_msg.n_payload] = malloc(strlen(nextline) + 1);
            if (log_msg.payload[log_msg.n_payload] == NULL) {
                log_printf(FATAL, "send_log_msg: Failed to malloc log message of length %d", strlen(nextline) + 1);
                exit(1);
            }
            strcpy(log_msg.payload[log_msg.n_payload], nextline);
            log_msg.n_payload++;
        } else if (feof(log_file) != 0) {  // All write ends of FIFO are closed, don't send any more logs
            // It's expected that all write ends are closed only when processes with logger_init() are killed
            if (log_msg.n_payload != 0) {  //if a few last logs to send, break to send those logs
                break;
            } else {  // log_msg.n_payload == 0;  (payload is empty) Return immediately
                free(log_msg.payload);
                return;
            }
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {  // No more to read on pipe (would block in a blocking read)
            break;
        } else {  // Error occurred
            log_printf(ERROR, "send_log_msg: Error reading from log fifo: %s", strerror(errno));
            // Free loaded payload contents and the payload itself
            for (int i = 0; i < log_msg.n_payload; i++) {
                free(log_msg.payload[i]);
            }
            free(log_msg.payload);
            return;
        }
    }

    //prepare the message for sending
    uint16_t len_pb = text__get_packed_size(&log_msg);
    uint8_t* send_buf = make_buf(LOG_MSG, len_pb);
    text__pack(&log_msg, send_buf + BUFFER_OFFSET);  //pack message into the rest of send_buf (starting at send_buf[3] onward)

    //send message on socket
    if (writen(conn_fd, send_buf, len_pb + BUFFER_OFFSET) == -1) {
        log_printf(ERROR, "send_log_msg: sending log message over socket failed: %s", strerror(errno));
    }

    //free all allocated memory
    for (int i = 0; i < log_msg.n_payload; i++) {
        free(log_msg.payload[i]);
    }
    free(log_msg.payload);
    free(send_buf);
}

/*
* Send a timestamp message over TCP with a timestamp attached to the message. The timestamp is when the net_handler on runtime has received the
* packet and processed it. 
* Arguments:
    - int conn_fd: socket connection's file descriptor on which to write to the TCP port
    - TimeStamps* dawn_timestamp_msg: Unpacked timestamp_proto from Dawn
*/
static void send_timestamp_msg(int conn_fd, TimeStamps* dawn_timestamp_msg) {
    dawn_timestamp_msg->runtime_timestamp = millis();
    uint16_t len_pb = time_stamps__get_packed_size(dawn_timestamp_msg);
    uint8_t* send_buf = make_buf(TIME_STAMP_MSG, len_pb);
    time_stamps__pack(dawn_timestamp_msg, send_buf + BUFFER_OFFSET);
    if (writen(conn_fd, send_buf, len_pb + BUFFER_OFFSET) == -1) {
        log_printf(ERROR, "send_timestamp_msg: sending log message over socket failed: %s", strerror(errno));
    }
    free(send_buf);
}

/*
 * Send a challenge data message on the TCP connection to the client. Reads packets from the UNIX socket from
 * executor until all messages are read, packages the message, and sends it.
 * Arguments:
 *    - int conn_fd: socket connection's file descriptor on which to write to the TCP port
 *    - int challenge_fd: Unix socket connection's file descriptor from which to read challenge results from executor
 */
static void send_challenge_results(int conn_fd, int challenge_fd) {
    // Get results from executor
    int buf_size = 256;
    char read_buf[buf_size];

    int read_len = recvfrom(challenge_fd, read_buf, buf_size, 0, NULL, NULL);
    if (read_len == buf_size) {
        log_printf(WARN, "send_challenge_results: read length matches size of read buffer %d", read_len);
    }
    if (read_len < 0) {
        log_printf(ERROR, "send_challenge_results: socket recv from challenge_fd failed: %s", strerror(errno));
        return;
    }

    // Send results to client
    if (writen(conn_fd, read_buf, read_len) == -1) {
        log_printf(ERROR, "send_challenge_results: sending challenge data message failed: %s", strerror(errno));
    }
}

/*
 * Receives new message from client on TCP connection and processes the message.
 * Arguments:
 *    - int conn_fd: socket connection's file descriptor from which to read the message
 *    - int results_fd: file descriptor of FIFO pipe to executor to which to write challenge input data if received
 * Returns: pointer to integer in which return status will be stored
 *      0 if message received and processed
 *     -1 if message could not be parsed because client disconnected and connection closed
 *     -2 if message could not be unpacked or other error
 */
static int recv_new_msg(int conn_fd, int challenge_fd) {
    net_msg_t msg_type;  //message type
    uint16_t len_pb;     //length of incoming serialized protobuf message
    uint8_t* buf;        //buffer to read raw data into

    int err = parse_msg(conn_fd, &msg_type, &len_pb, &buf);
    if (err == 0) {  // Means there is EOF while reading which means client disconnected
        return -1;
    } else if (err == -1) {  // Means there is some other error while reading
        return -2;
    }
    //unpack according to message
    if (msg_type == CHALLENGE_DATA_MSG) {
        //socket address structure for the UNIX socket to executor for challenge data
        struct sockaddr_un exec_addr = {0};
        exec_addr.sun_family = AF_UNIX;
        strcpy(exec_addr.sun_path, CHALLENGE_SOCKET);

        int send_len = sendto(challenge_fd, buf, len_pb, 0, (struct sockaddr*) &exec_addr, sizeof(struct sockaddr_un));
        if (send_len < 0) {
            log_printf(ERROR, "recv_new_msg: socket send to challenge_fd failed: %s", strerror(errno));
            return -2;
        }
        if (send_len != len_pb) {
            log_printf(WARN, "recv_new_msg: socket send len %d is not equal to intended protobuf length %d", send_len, len_pb);
        }

        log_printf(DEBUG, "entering CHALLENGE mode. running coding challenges!");
        robot_desc_write(RUN_MODE, CHALLENGE);
    } else if (msg_type == RUN_MODE_MSG) {
        RunMode* run_mode_msg = run_mode__unpack(NULL, len_pb, buf);
        if (run_mode_msg == NULL) {
            log_printf(ERROR, "recv_new_msg: Cannot unpack run_mode msg");
            return -2;
        }

        //if shepherd is connected and dawn tries to set RUN_MODE == AUTO or TELEOP, block it
        if (pthread_self() == dawn_tid && robot_desc_read(SHEPHERD) == CONNECTED &&
            (run_mode_msg->mode == MODE__AUTO || (run_mode_msg->mode == MODE__TELEOP))) {
            log_printf(INFO, "You cannot send Robot to Auto or Teleop from Dawn with Shepherd connected!");
            return 0;
        }

        //write the specified run mode to the RUN_MODE field of the robot description
        switch (run_mode_msg->mode) {
            case (MODE__IDLE):
                log_printf(DEBUG, "entering IDLE mode");
                robot_desc_write(RUN_MODE, IDLE);
                break;
            case (MODE__AUTO):
                log_printf(DEBUG, "entering AUTO mode");
                robot_desc_write(RUN_MODE, AUTO);
                break;
            case (MODE__TELEOP):
                log_printf(DEBUG, "entering TELEOP mode");
                robot_desc_write(RUN_MODE, TELEOP);
                break;
            case (MODE__ESTOP):
                log_printf(DEBUG, "ESTOP RECEIVED! entering IDLE mode");
                robot_desc_write(RUN_MODE, IDLE);
                break;
            default:
                log_printf(ERROR, "recv_new_msg: requested robot to enter invalid robot mode %s", run_mode_msg->mode);
                break;
        }
        run_mode__free_unpacked(run_mode_msg, NULL);
    } else if (msg_type == START_POS_MSG) {
        StartPos* start_pos_msg = start_pos__unpack(NULL, len_pb, buf);
        if (start_pos_msg == NULL) {
            log_printf(ERROR, "recv_new_msg: Cannot unpack start_pos msg");
            return -2;
        }

        //write the specified start pos to the STARTPOS field of the robot description
        switch (start_pos_msg->pos) {
            case (POS__LEFT):
                log_printf(DEBUG, "robot is in LEFT start position");
                robot_desc_write(START_POS, LEFT);
                break;
            case (POS__RIGHT):
                log_printf(DEBUG, "robot is in RIGHT start position");
                robot_desc_write(START_POS, RIGHT);
                break;
            default:
                log_printf(ERROR, "recv_new_msg: trying to enter unknown start position %d", start_pos_msg->pos);
                break;
        }
        start_pos__free_unpacked(start_pos_msg, NULL);
    } else if (msg_type == DEVICE_DATA_MSG) {
        DevData* dev_data_msg = dev_data__unpack(NULL, len_pb, buf);
        if (dev_data_msg == NULL) {
            log_printf(ERROR, "recv_new_msg: Cannot unpack dev_data msg");
            return -2;
        }
        // For each device, place sub requests for the requested parameters
        for (int i = 0; i < dev_data_msg->n_devices; i++) {
            Device* req_device = dev_data_msg->devices[i];
            uint32_t requests = 0;
            for (int j = 0; j < req_device->n_params; j++) {
                // Place a sub request for each parameter that has its boolean value set to 1
                if (req_device->params[j]->val_case == PARAM__VAL_BVAL) {
                    requests |= (req_device->params[j]->bval << j);
                }
            }
            int err = place_sub_request(req_device->uid, NET_HANDLER, requests);
            if (err == -1) {
                log_printf(ERROR, "recv_new_msg: Invalid device subscription, device uid %llu is invalid", req_device->uid);
            }
        }
        dev_data__free_unpacked(dev_data_msg, NULL);
    } else if (msg_type == GAME_STATE_MSG) {
        GameState* game_state_msg = game_state__unpack(NULL, len_pb, buf);
        if (game_state_msg == NULL) {
            log_printf(ERROR, "recv_new_msg: Cannot unpack game_state msg");
            return -2;
        }
        switch (game_state_msg->state) {
            case (STATE__POISON_IVY):
                log_printf(DEBUG, "entering POISON_IVY state");
                robot_desc_write(POISON_IVY, ACTIVE);
                break;
            case (STATE__DEHYDRATION):
                log_printf(DEBUG, "entering DEHYDRATION state");
                robot_desc_write(DEHYDRATION, ACTIVE);
                break;
            case (STATE__HYPOTHERMIA_START):
                log_printf(DEBUG, "entering HYPOTHERMIA state");
                robot_desc_write(HYPOTHERMIA, ACTIVE);
                break;
            case (STATE__HYPOTHERMIA_END):
                log_printf(DEBUG, "exiting HYPOTHERMIA state");
                robot_desc_write(HYPOTHERMIA, INACTIVE);
                break;
            default:
                log_printf(ERROR, "requested gamestate to enter invalid state %s", game_state_msg->state);
                game_state__free_unpacked(game_state_msg, NULL);
        }
    } else if (msg_type == TIME_STAMP_MSG) {
        TimeStamps* time_stamp_msg = time_stamps__unpack(NULL, len_pb, buf);
        if (time_stamp_msg == NULL) {
            log_printf(ERROR, "recv_new_msg: Cannot unpack time_stamp msg");
        }
        send_timestamp_msg(conn_fd, time_stamp_msg);
        time_stamps__free_unpacked(time_stamp_msg, NULL);
    } else {
        log_printf(ERROR, "recv_new_msg: unknown message type %d, tcp should only receive CHALLENGE_DATA (2), RUN_MODE (0), START_POS (1), or DEVICE_DATA (4)", msg_type);
        return -2;
    }
    free(buf);
    return 0;
}


/*
 * Main control loop for a TCP connection. Sets up connection by opening up pipe to read log messages from
 * and sets up read_set for select(). Then it runs main control loop, using select() to make actions event-driven.
 * Arguments:
 *    - void *args: arguments containing file descriptors and file pointers that need to be closed on exit (and other settings)
 * Return:
 *    - NULL
 */
static void* tcp_process(void* tcp_args) {
    tcp_conn_args_t* args = (tcp_conn_args_t*) tcp_args;
    pthread_cleanup_push(tcp_conn_cleanup, args);
    int ret;

    //variables used for waiting for something to do using select()
    fd_set read_set;
    int log_fd;
    int maxfd = args->challenge_fd > args->conn_fd ? args->challenge_fd : args->conn_fd;
    if (args->send_logs) {
        log_fd = fileno(args->log_file);
        maxfd = log_fd > maxfd ? log_fd : maxfd;
    }
    maxfd = maxfd + 1;

    //main control loop that is responsible for sending and receiving data
    while (1) {
        //set up the read_set argument to selct()
        FD_ZERO(&read_set);
        FD_SET(args->conn_fd, &read_set);
        FD_SET(args->challenge_fd, &read_set);
        if (args->send_logs) {
            FD_SET(log_fd, &read_set);
        }

        //prepare to accept cancellation requests over the select
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        //wait for something to happen
        if (select(maxfd, &read_set, NULL, NULL, NULL) < 0) {
            log_printf(ERROR, "tcp_process: Failed to wait for select in control loop for client %d: %s", args->client, strerror(errno));
        }

        //deny all cancellation requests until the next loop
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        // If client wants logs, logs are availble to send, and FIFO doesn't have an EOF, send logs
        if (args->send_logs && FD_ISSET(log_fd, &read_set)) {
            send_log_msg(args->conn_fd, args->log_file);
        }

        //send challenge results if executor sent them
        if (FD_ISSET(args->challenge_fd, &read_set)) {
            send_challenge_results(args->conn_fd, args->challenge_fd);
        }

        //receive new message on socket if it is ready
        if (FD_ISSET(args->conn_fd, &read_set)) {
            if ((ret = recv_new_msg(args->conn_fd, args->challenge_fd)) != 0) {
                if (ret == -1) {
                    log_printf(DEBUG, "client %d has disconnected", args->client);
                    break;
                } else {
                    log_printf(ERROR, "error parsing message from client %d", args->client);
                }
            }
        }
    }

    //call the cleanup function
    pthread_cleanup_pop(1);
    return NULL;
}


/************************ PUBLIC FUNCTIONS *************************/


void start_tcp_conn(robot_desc_field_t client, int conn_fd, int send_logs) {
    pthread_t* tid;
    if (client == DAWN) {
        tid = &dawn_tid;
    } else if (client == SHEPHERD) {
        tid = &shepherd_tid;
    } else {
        log_printf(ERROR, "start_tcp_conn: Invalid TCP client %d, not DAWN(%d) or SHEPHERD(%d)", client, DAWN, SHEPHERD);
        return;
    }

    //initialize argument to new connection thread
    tcp_conn_args_t* args = malloc(sizeof(tcp_conn_args_t));
    if (args == NULL) {
        log_printf(FATAL, "start_tcp_conn: Failed to malloc args");
        exit(1);
    }
    args->client = client;
    args->conn_fd = conn_fd;
    args->send_logs = send_logs;
    args->log_file = NULL;
    args->challenge_fd = -1;

    // open challenge socket to read and write
    if ((args->challenge_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        log_printf(ERROR, "start_tcp_conn: failed to create challenge socket: %s", strerror(errno));
        return;
    }
    struct sockaddr_un my_addr = {0};
    my_addr.sun_family = AF_UNIX;
    if (bind(args->challenge_fd, (struct sockaddr*) &my_addr, sizeof(sa_family_t)) < 0) {
        log_printf(FATAL, "start_tcp_conn: challenge socket bind failed: %s", strerror(errno));
        close(args->challenge_fd);
        return;
    }

    //Open FIFO pipe for logs
    if (send_logs) {
        int log_fd;
        if ((log_fd = open(LOG_FIFO, O_RDONLY | O_NONBLOCK)) == -1) {
            log_printf(ERROR, "start_tcp_conn: could not open log FIFO on %d: %s", args->client, strerror(errno));
            close(args->challenge_fd);
            return;
        }
        if ((args->log_file = fdopen(log_fd, "r")) == NULL) {
            log_printf(ERROR, "start_tcp_conn: could not open log file from fd: %s", strerror(errno));
            close(args->challenge_fd);
            close(log_fd);
            return;
        }
    }

    //create the main control thread for this client
    if (pthread_create(tid, NULL, tcp_process, args) != 0) {
        log_printf(ERROR, "start_tcp_conn: Failed to create main TCP thread for %d: %s", client, strerror(errno));
        return;
    }
    log_printf(DEBUG, "Successfully initialized TCP connection with client %d\n", client);
    robot_desc_write(client, CONNECTED);
}


void stop_tcp_conn(robot_desc_field_t client) {
    if (client != DAWN && client != SHEPHERD) {
        log_printf(ERROR, "stop_tcp_conn: Invalid TCP client %d, not DAWN(%d) or SHEPHERD(%d)", client, DAWN, SHEPHERD);
        return;
    }

    pthread_t tid = (client == DAWN) ? dawn_tid : shepherd_tid;

    if (pthread_cancel(tid) != 0) {
        log_printf(ERROR, "stop_tcp_conn: Failed to cancel TCP client thread for %d: %s", client, strerror(errno));
    }
    if (pthread_join(tid, NULL) != 0) {
        log_printf(ERROR, "stop_tcp_conn: Failed to join on TCP client thread for %d: %s", client, strerror(errno));
    }
}
