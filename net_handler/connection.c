#include "connection.h"

//used for creating and cleaning up TCP connection
typedef struct {
    int conn_fd;
	pthread_mutex_t conn_lock;
    int send_logs;
    FILE* log_file;
	uint64_t last_sent_status;
    robot_desc_field_t client;
} conn_args_t;

// For each client, we have two threads
// 1) Events thread (Acts on messages we receive from them)
// 2) Poll thread (On an interval, send info to the client)
pthread_t dawn_events_tid, dawn_poll_tid, shepherd_events_tid, shepherd_poll_tid;

// The start time of when the tcp connection was created with Dawn
uint64_t dawn_start_time = -1;

// Flags for sending runtime status on a connection, and locks over the flags
uint8_t dawn_send_status = 1, shep_send_status = 1;
pthread_mutex_t dawn_status_lock = PTHREAD_MUTEX_INITIALIZER, shep_status_lock = PTHREAD_MUTEX_INITIALIZER;

// Number of ms between sending each DeviceData to Dawn
#define DEVICE_DATA_INTERVAL 50 // 20 Hz

// Maximum number of ms between sending each Runtime Status message
#define STATUS_INTERVAL 10000

/*
 * Gets the status flag of the specified client, acquiring and releasing the appropriate lock
 * Arguments:
 *    - calling_client: client from which this function is called, and whose status flag will be returned
 * Returns:
 *    - 0 or 1, the value of the corresponding status flag
 */
static uint8_t get_status_flag(robot_desc_field_t calling_client) {
	uint8_t ret;
	uint8_t* flag = (calling_client == DAWN) ? &dawn_send_status : &shep_send_status;
	pthread_mutex_t* lock = (calling_client == DAWN) ? &dawn_status_lock : &shep_status_lock;
	
	// acquire lock, get the desired value, then release lock
	if (pthread_mutex_lock(lock) != 0) {
		log_printf(ERROR, "get_status_flag: error acquiring status lock: %s", strerror(errno));
	}
	ret = *flag;
	if (pthread_mutex_unlock(lock) != 0) {
		log_printf(ERROR, "get_status_flag: error releasing status lock: %s", strerror(errno));
	}
	
	return ret;
}

/*
 * Sets the status flag of the specified client to val, acquiring and releasing the appropriate lock.
 * Sends a runtime status message on the specified connfd if val = 0; updates conn_args->last_status_time
 * Arguments:
 *    - conn_args: the conn_args_t associated with the calling thread which provides information for the function
 *    - other: 0 or 1; 0 if we want to set our own status flag, 1 if we want to set the other status flag
 *    - val: 0 or 1; the value to which to set the corresponding status flag
 */
static void set_status_flag(conn_args_t* conn_args, uint8_t other, uint8_t val) {
	// figure out which flag and lock we want to set
	uint8_t* flag;
	pthread_mutex_t* lock;
	if (conn_args->client == DAWN) {
		flag = (other) ? &shep_send_status : &dawn_send_status;
		lock = (other) ? &shep_status_lock : &dawn_status_lock;
	} else {
		flag = (other) ? &dawn_send_status : &shep_send_status;
		lock = (other) ? &dawn_status_lock : &shep_status_lock;
	}
	
	// acquire lock, set desired value, then release lock
	if (pthread_mutex_lock(lock) != 0) {
		log_printf(ERROR, "set_status_flag: error acquiring status lock: %s", strerror(errno));
	}
	*flag = val;
	
	// if we are setting flag to 0, this means we need to send a runtime status message before unlocking the flag mutex
	if (val == 0) {
		send_status_msg(conn_args->conn_fd, &(conn_args->conn_lock));
		conn_args->last_sent_status = millis();
	}
	
	if (pthread_mutex_unlock(lock) != 0) {
		log_printf(ERROR, "set_status_flag: error releasing status lock: %s", strerror(errno));
	}
}

/*
 * Clean up memory and file descriptors before exiting from tcp_process
 * Arguments:
 *    - void *args: should be a pointer to tcp_conn_args_t populated with the listed descriptors and pointers
 */
static void conn_cleanup(void* args) {
    conn_args_t* conn_args = (conn_args_t *) args;
    robot_desc_write(RUN_MODE, IDLE);

    if (close(conn_args->conn_fd) != 0) {
        log_printf(ERROR, "Failed to close conn_fd: %s", strerror(errno));
    }
    if (conn_args->log_file != NULL) {
        if (fclose(conn_args->log_file) != 0) {
            log_printf(ERROR, "Failed to close log_file: %s", strerror(errno));
        }
        conn_args->log_file = NULL;
    }
    robot_desc_write(conn_args->client, DISCONNECTED);
    if (conn_args->client == DAWN) {
        // Disconnect inputs if Dawn is no longer connected
        robot_desc_write(GAMEPAD, DISCONNECTED);
        robot_desc_write(KEYBOARD, DISCONNECTED);
    }
    free(args);
}

/*
 * A thread function to handle messages with a specific client. 
 * This thread waits until a message can be read, reads it, then responds accordingly.
 * It is one of two main control loops for a connection. Sets up connection by opening up pipe to read log messages from
 * and sets up read_set for select(). Then it runs main control loop, using select() to make actions event-driven.
 * Arguments:
 *    - void *tcp_args: arguments containing file descriptors and file pointers that need to be closed on exit (and other settings)
 * Return:
 *    - NULL
 */
static void* events_thread(void* args) {
    conn_args_t* conn_args = (conn_args_t*) args;
    pthread_cleanup_push(conn_cleanup, args);
    int ret;

    //variables used for waiting for something to do using select()
    fd_set read_set;
    int log_fd;
    int maxfd = conn_args->conn_fd;
    if (conn_args->send_logs) {
        log_fd = fileno(conn_args->log_file);
        maxfd = log_fd > maxfd ? log_fd : maxfd;
    }
    maxfd = maxfd + 1;

    //main control loop that is responsible for sending and receiving data
    while (1) {
        //set up the read_set argument to select()
        FD_ZERO(&read_set);
        FD_SET(conn_args->conn_fd, &read_set);
        if (conn_args->send_logs) {
            FD_SET(log_fd, &read_set);
        }

        //prepare to accept cancellation requests over the select
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        //wait for something to happen
        if (select(maxfd, &read_set, NULL, NULL, NULL) < 0) {
            log_printf(ERROR, "events_thread: Failed to wait for select in control loop for client %d: %s", conn_args->client, strerror(errno));
        }

        //deny all cancellation requests until the next loop
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        // If client wants logs, logs are availble to send, and FIFO doesn't have an EOF, send logs
        if (conn_args->send_logs && FD_ISSET(log_fd, &read_set)) {
            send_log_msg(conn_args->conn_fd, conn_args->log_file, &(conn_args->conn_lock));
        }

        //receive new message on socket if it is ready
        if (FD_ISSET(conn_args->conn_fd, &read_set)) {
            if ((ret = recv_new_msg(conn_args->conn_fd, conn_args->client, &(conn_args->conn_lock))) < 0) {
                if (ret == -1) {
                    log_printf(DEBUG, "events_thread: client %d has disconnected", conn_args->client);
                    break;
                } else {
                    log_printf(ERROR, "events_thread: error parsing message from client %d", conn_args->client);
                }
            } else {
				// if we received a message for which we should send a status update, send a status update
            	if (ret == (int) RUN_MODE_MSG || ret == (int) START_POS_MSG || ret == (int) GAME_STATE_MSG) {
					set_status_flag(conn_args, 0, 0); // set our status flag to 0, send status message, and update last_sent_status
            	}
				set_status_flag(conn_args, 1, 1); // tell the other thread to send a status message as well
            }
        }
    }

    //call the cleanup function
    pthread_cleanup_pop(1);
    return NULL;
}

/*
 * A thread function to handle messages with a specific client.
 * This thread creates a new message on an interval, and sends it to the client.
 * it is one of the two main control loops for a connection.
 * Arguments:
 * - args: arguments containing file descriptors and file pointers that need to be closed on exit (and other settings)
 * Return:
 * - NULL
 */
static void* poll_thread(void* args) {
    conn_args_t* conn_args = (conn_args_t*) args;
    uint64_t time = millis();
    uint64_t last_sent_device_data = 0;
	
    while (1) {
        // Update time
        time = millis();
        // If enough time has passed, send a new DeviceData to Dawn
        if (conn_args->client == DAWN) {
            if (time - last_sent_device_data > DEVICE_DATA_INTERVAL) {
                // Send a DEVICE_DATA to client
                send_device_data(conn_args->conn_fd, dawn_start_time, &(conn_args->conn_lock));
                last_sent_device_data = time;
            }
        }
        
		// If other thead has signaled this thread to send a status message, or 
		// enough time has passed since the last status message, send runtime status on connection
		if (get_status_flag(conn_args->client) || (time - conn_args->last_sent_status > STATUS_INTERVAL)) {
			// set our flag to 0, send status message, update last_sent_status
			set_status_flag(conn_args, 0, 0);
		}
		
        pthread_testcancel();  // Make sure we allow cancelling this thread at some point
        usleep(40000);         // Loop throttling
    }
    return NULL;
}

/************************ PUBLIC FUNCTIONS *************************/

void start_conn(robot_desc_field_t client, int conn_fd, int send_logs) {
    if (client != DAWN && client != SHEPHERD) {
        log_printf(ERROR, "start_conn: Invalid client %d, not DAWN(%d) or SHEPHERD(%d)", client, DAWN, SHEPHERD);
        return;
    }

    //initialize argument to new connection thread
    conn_args_t* args = malloc(sizeof(conn_args_t));
    if (args == NULL) {
        log_printf(FATAL, "start_conn: Failed to malloc args");
        exit(1);
    }
    args->client = client;
    args->conn_fd = conn_fd;
    args->send_logs = send_logs;
    args->log_file = NULL;
	if (pthread_mutex_init(&(args->conn_lock), NULL) != 0) {
		log_printf(ERROR, "start_conn: Failed to init the connection mutex: %s", strerror(errno));
	}

    //Open FIFO pipe for logs
    if (send_logs) {
        int log_fd;
        if ((log_fd = open(LOG_FIFO, O_RDONLY | O_NONBLOCK)) == -1) {
            log_printf(ERROR, "start_conn: could not open log FIFO on %d: %s", args->client, strerror(errno));
            return;
        }
        if ((args->log_file = fdopen(log_fd, "r")) == NULL) {
            log_printf(ERROR, "start_conn: could not open log file from fd: %s", strerror(errno));
            close(log_fd);
            return;
        }
    }

    // Update the start time of the TCP connection with Dawn
    if (client == DAWN) {
        dawn_start_time = millis();
    }

    //create the main control threads for this client
    if (pthread_create((client == DAWN) ? &dawn_events_tid : &shepherd_events_tid, NULL, events_thread, args) != 0) {
        log_printf(ERROR, "start_conn: Failed to create main events_thread for %d: %s", client, strerror(errno));
        return;
    }
    if (pthread_create((client == DAWN) ? &dawn_poll_tid : &shepherd_poll_tid, NULL, poll_thread, args) != 0) {
        log_printf(ERROR, "start_conn: Failed to create main poll_thread for %d: %s", client, strerror(errno));
        return;
    }
    robot_desc_write(client, CONNECTED);
}

void stop_conn(robot_desc_field_t client) {
    if (client != DAWN && client != SHEPHERD) {
        log_printf(ERROR, "stop_conn: Invalid TCP client %d, not DAWN(%d) or SHEPHERD(%d)", client, DAWN, SHEPHERD);
        return;
    }

    // Cancel and join the poll thread
    // Note: Since both the poll and events thread use the same connection socket,
    // The first thread we cancel should *not* clean up the connection, but the second thread we cancel should
    if (pthread_cancel((client == DAWN) ? dawn_poll_tid : shepherd_poll_tid) != 0) {
        log_printf(ERROR, "stop_conn: Failed to cancel poll thread for %d: %s", client, strerror(errno));
    }
    if (pthread_join((client == DAWN) ? dawn_poll_tid : shepherd_poll_tid, NULL) != 0) {
        log_printf(ERROR, "stop_conn: Failed to join on poll thread for %d: %s", client, strerror(errno));
    }
    // Cancel and join the events thread
    if (pthread_cancel((client == DAWN) ? dawn_events_tid : shepherd_events_tid) != 0) {
        log_printf(ERROR, "stop_conn: Failed to cancel events thread for %d: %s", client, strerror(errno));
    }
    if (pthread_join((client == DAWN) ? dawn_events_tid : shepherd_events_tid, NULL) != 0) {
        log_printf(ERROR, "stop_conn: Failed to join on events thread for %d: %s", client, strerror(errno));
    }
}
