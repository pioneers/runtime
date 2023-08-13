#include "connection.h"

// used for creating and cleaning up TCP connection
typedef struct {
    int conn_fd;
    int send_logs;
    FILE* log_file;
    robot_desc_field_t client;
} tcp_conn_args_t;

// For each client, we have two threads
// 1) Events thread (Acts on messages we receive from them)
// 2) Poll thread (On an interval, send info to the client)
pthread_t dawn_events_tid, dawn_poll_tid, shepherd_events_tid, shepherd_poll_tid;

// The start time of when the tcp connection was created with Dawn
uint64_t dawn_start_time = -1;

// Number of ms between sending each DeviceData to Dawn
#define DEVICE_DATA_INTERVAL 10

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
 * A thread function to handle messages with a specific client.
 * This thread waits until a message can be read, reads it, then responds accordingly.
 * It is one of two main control loops for a TCP connection. Sets up connection by opening up pipe to read log messages from
 * and sets up read_set for select(). Then it runs main control loop, using select() to make actions event-driven.
 * Arguments:
 *    - void *tcp_args: arguments containing file descriptors and file pointers that need to be closed on exit (and other settings)
 * Return:
 *    - NULL
 */
static void* tcp_events_thread(void* tcp_args) {
    tcp_conn_args_t* args = (tcp_conn_args_t*) tcp_args;
    pthread_cleanup_push(tcp_conn_cleanup, args);
    int ret;

    // variables used for waiting for something to do using select()
    fd_set read_set;
    int log_fd;
    int maxfd = args->conn_fd;
    if (args->send_logs) {
        log_fd = fileno(args->log_file);
        maxfd = log_fd > maxfd ? log_fd : maxfd;
    }
    maxfd = maxfd + 1;

    // main control loop that is responsible for sending and receiving data
    while (1) {
        // set up the read_set argument to select()
        FD_ZERO(&read_set);
        FD_SET(args->conn_fd, &read_set);
        if (args->send_logs) {
            FD_SET(log_fd, &read_set);
        }

        // prepare to accept cancellation requests over the select
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        // wait for something to happen
        if (select(maxfd, &read_set, NULL, NULL, NULL) < 0) {
            log_printf(ERROR, "tcp_process: Failed to wait for select in control loop for client %d: %s", args->client, strerror(errno));
        }

        // deny all cancellation requests until the next loop
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        // If client wants logs, logs are availble to send, and FIFO doesn't have an EOF, send logs
        if (args->send_logs && FD_ISSET(log_fd, &read_set)) {
            send_log_msg(args->conn_fd, args->log_file);
        }

        // receive new message on socket if it is ready
        if (FD_ISSET(args->conn_fd, &read_set)) {
            if ((ret = recv_new_msg(args->conn_fd, args->client)) != 0) {
                if (ret == -1) {
                    log_printf(DEBUG, "client %d has disconnected", args->client);
                    break;
                } else {
                    log_printf(ERROR, "error parsing message from client %d", args->client);
                }
            }
        }
    }

    // call the cleanup function
    pthread_cleanup_pop(1);
    return NULL;
}

/*
 * A thread function to handle messages with a specific client.
 * This thread creates a new message on an interval, and sends it to the client.
 * it is one of the two main control loops for a TCP connection.
 * Arguments:
 * - tcp_args: arguments containing file descriptors and file pointers that need to be closed on exit (and other settings)
 * Return:
 * - NULL
 */
static void* tcp_poll_thread(void* tcp_args) {
    tcp_conn_args_t* args = (tcp_conn_args_t*) tcp_args;
    uint64_t time = millis();
    uint64_t last_sent_device_data = 0;
    while (1) {
        // Update time
        time = millis();
        // If enough time has passed, send a new DeviceData to Dawn
        if (args->client == DAWN) {
            if (time - last_sent_device_data > DEVICE_DATA_INTERVAL) {
                // Send a DEVICE_DATA to client
                send_device_data(args->conn_fd, dawn_start_time);
                last_sent_device_data = time;
            }
        }
        // If enough time has passed, send a Runtime status message
        // TODO: @Ben/Daniel
        pthread_testcancel();  // Make sure we allow cancelling this thread at some point
        usleep(40000);         // Loop throttling
    }
    return NULL;
}

/************************ PUBLIC FUNCTIONS *************************/


void start_tcp_conn(robot_desc_field_t client, int conn_fd, int send_logs, uint8_t* buf, uint16_t len_pb) {
    if (client != DAWN && client != SHEPHERD) {
        log_printf(ERROR, "start_tcp_conn: Invalid TCP client %d, not DAWN(%d) or SHEPHERD(%d)", client, DAWN, SHEPHERD);
        return;
    }

    // initialize argument to new connection thread
    tcp_conn_args_t* args = malloc(sizeof(tcp_conn_args_t));
    if (args == NULL) {
        log_printf(FATAL, "start_tcp_conn: Failed to malloc args");
        exit(1);
    }
    args->client = client;
    args->conn_fd = conn_fd;
    args->send_logs = send_logs;
    args->log_file = NULL;

    // Open FIFO pipe for logs
    if (send_logs) {
        int log_fd;
        if ((log_fd = open(LOG_FIFO, O_RDONLY | O_NONBLOCK)) == -1) {
            log_printf(ERROR, "start_tcp_conn: could not open log FIFO on %d: %s", args->client, strerror(errno));
            return;
        }
        if ((args->log_file = fdopen(log_fd, "r")) == NULL) {
            log_printf(ERROR, "start_tcp_conn: could not open log file from fd: %s", strerror(errno));
            close(log_fd);
            return;
        }
    }

    // Update the start time of the TCP connection with Dawn
    if (client == DAWN) {
        if (process_security_msg(conn_fd, buf, len_pb) != 0){
            log_printf(ERROR, "start_tcp_conn: Failed to establish connection with DAWN");
            return;
        }
        if (process_security_msg(conn_fd, buf, len_pb) != 0){
            log_printf(ERROR, "start_tcp_conn: Failed to establish connection with DAWN");
            return;
        }

        dawn_start_time = millis();
    } else if (client == SHEPHERD) {
        if (process_security_msg(conn_fd, buf, len_pb) != 0){
            log_printf(ERROR, "start_tcp_conn: Failed to establish connection with SHEPHERD");
            return;
        }
        if (process_security_msg(conn_fd, buf, len_pb) != 0){
            log_printf(ERROR, "start_tcp_conn: Failed to establish connection with SHEPHERD");
            return;
        }
    }

    // create the main control threads for this client
    if (pthread_create((client == DAWN) ? &dawn_events_tid : &shepherd_events_tid, NULL, tcp_events_thread, args) != 0) {
        log_printf(ERROR, "start_tcp_conn: Failed to create main TCP thread tcp_events for %d: %s", client, strerror(errno));
        return;
    }
    if (pthread_create((client == DAWN) ? &dawn_poll_tid : &shepherd_poll_tid, NULL, tcp_poll_thread, args) != 0) {
        log_printf(ERROR, "start_tcp_conn: Failed to create main TCP thread tcp_poll for %d: %s", client, strerror(errno));
        return;
    }
    robot_desc_write(client, CONNECTED);
}


void stop_tcp_conn(robot_desc_field_t client) {
    if (client != DAWN && client != SHEPHERD) {
        log_printf(ERROR, "stop_tcp_conn: Invalid TCP client %d, not DAWN(%d) or SHEPHERD(%d)", client, DAWN, SHEPHERD);
        return;
    }

    // Cancel and join the poll thread
    // Note: Since both the poll and events thread use the same connection socket,
    // The first thread we cancel should *not* clean up the connection, but the second thread we cancel should
    if (pthread_cancel((client == DAWN) ? dawn_poll_tid : shepherd_poll_tid) != 0) {
        log_printf(ERROR, "stop_tcp_conn: Failed to cancel TCP client poll thread for %d: %s", client, strerror(errno));
    }
    if (pthread_join((client == DAWN) ? dawn_poll_tid : shepherd_poll_tid, NULL) != 0) {
        log_printf(ERROR, "stop_tcp_conn: Failed to join on TCP client poll thread for %d: %s", client, strerror(errno));
    }
    // Cancel and join the events thread
    if (pthread_cancel((client == DAWN) ? dawn_events_tid : shepherd_events_tid) != 0) {
        log_printf(ERROR, "stop_tcp_conn: Failed to cancel TCP client events thread for %d: %s", client, strerror(errno));
    }
    if (pthread_join((client == DAWN) ? dawn_events_tid : shepherd_events_tid, NULL) != 0) {
        log_printf(ERROR, "stop_tcp_conn: Failed to join on TCP client events thread for %d: %s", client, strerror(errno));
    }
}
