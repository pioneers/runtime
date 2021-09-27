#include "message.h"

// ******************************************* SEND MESSAGES ***************************************** //

/*
 * Send a log message on the TCP connection to the client. Reads lines from the pipe until there is no more data
 * or it has read MAX_NUM_LOGS lines from the pipe, packages the message, and sends it.
 * Arguments:
 *    - int conn_fd: socket connection's file descriptor on which to write to the TCP port
 *    - pthread_mutex_t* conn_lock: lock over the connection socket
 */
void send_log_msg(int conn_fd, FILE* log_file, pthread_mutex_t* conn_lock) {
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

	// acquire lock
	if (pthread_mutex_lock(conn_lock) != 0) {
		log_printf(ERROR, "send_log_msg: failed to acquire lock: %s", strerror(errno));
	}

    //send message on socket
    if (writen(conn_fd, send_buf, len_pb + BUFFER_OFFSET) == -1) {
        log_printf(ERROR, "send_log_msg: sending log message over socket failed: %s", strerror(errno));
    }
	
	// release lock
	if (pthread_mutex_unlock(conn_lock) != 0) {
		log_printf(ERROR, "send_log_msg: failed to release lock: %s", strerror(errno));
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
 *    - int conn_fd: socket connection's file descriptor on which to write to the TCP port
 *    - pthread_mutex_t* conn_lock: lock over the connection socket
 *    - TimeStamps* dawn_timestamp_msg: Unpacked timestamp_proto from Dawn 
 */
void send_timestamp_msg(int conn_fd, TimeStamps* dawn_timestamp_msg, pthread_mutex_t* conn_lock) {
    dawn_timestamp_msg->runtime_timestamp = millis();
    uint16_t len_pb = time_stamps__get_packed_size(dawn_timestamp_msg);
    uint8_t* send_buf = make_buf(TIME_STAMP_MSG, len_pb);
    time_stamps__pack(dawn_timestamp_msg, send_buf + BUFFER_OFFSET);
	
	// acquire lock
	if (pthread_mutex_lock(conn_lock) != 0) {
		log_printf(ERROR, "send_timestamp_msg: failed to acquire lock: %s", strerror(errno));
	}
	
	// send message on socket
    if (writen(conn_fd, send_buf, len_pb + BUFFER_OFFSET) == -1) {
        log_printf(ERROR, "send_timestamp_msg: sending log message over socket failed: %s", strerror(errno));
    }
    free(send_buf);
	
	// release lock
	if (pthread_mutex_unlock(conn_lock) != 0) {
		log_printf(ERROR, "send_timestamp_msg: failed to release lock: %s", strerror(errno));
	}
}

/**
 * Sends a Device Data message to Dawn.
 * Arguments:
 *    - int dawn_socket_fd: socket fd for Dawn connection
 *    - uint64_t dawn_start_time: time that Dawn connection started, for calculating time in CustomData
 *    - pthread_mutex_t* conn_lock: lock over the connection socket
 */
void send_device_data(int dawn_socket_fd, uint64_t dawn_start_time, pthread_mutex_t* conn_lock) {
    int len_pb;
    uint8_t* buffer;

    dev_id_t dev_ids[MAX_DEVICES];
    int valid_dev_idxs[MAX_DEVICES];
    uint32_t catalog;

    param_val_t custom_params[UCHAR_MAX];
    param_type_t custom_types[UCHAR_MAX];
    char custom_names[UCHAR_MAX][LOG_KEY_LENGTH];
    uint8_t num_params;

    DevData dev_data = DEV_DATA__INIT;

    //get information
    get_catalog(&catalog);
    get_device_identifiers(dev_ids);

    //calculate num_devices, get valid device indices
    int num_devices = 0;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (catalog & (1 << i)) {
            valid_dev_idxs[num_devices] = i;
            num_devices++;
        }
    }
    dev_data.devices = malloc((num_devices + 1) * sizeof(Device*));  // + 1 is for custom data
    if (dev_data.devices == NULL) {
        log_printf(FATAL, "send_device_data: Failed to malloc");
        exit(1);
    }

    //populate dev_data.device[i]
    int dev_idx = 0;
    for (int i = 0; i < num_devices; i++) {
        int idx = valid_dev_idxs[i];
        device_t* device_info = get_device(dev_ids[idx].type);
        if (device_info == NULL) {
            log_printf(ERROR, "send_device_data: Device %d in SHM with type %d is invalid", idx, dev_ids[idx].type);
            continue;
        }

        Device* device = malloc(sizeof(Device));
        if (device == NULL) {
            log_printf(FATAL, "send_device_data: Failed to malloc");
            exit(1);
        }
        device__init(device);
        dev_data.devices[dev_idx] = device;
        device->type = dev_ids[idx].type;
        device->uid = dev_ids[idx].uid;
        device->name = device_info->name;

        device->n_params = 0;
        param_val_t param_data[MAX_PARAMS];
        device_read_uid(device->uid, NET_HANDLER, DATA, get_readable_param_bitmap(device->type), param_data);

        device->params = malloc(device_info->num_params * sizeof(Param*));
        if (device->params == NULL) {
            log_printf(FATAL, "send_device_data: Failed to malloc");
            exit(1);
        }
        //populate device parameters
        for (int j = 0; j < device_info->num_params; j++) {
            Param* param = malloc(sizeof(Param));
            if (param == NULL) {
                log_printf(FATAL, "send_device_data: Failed to malloc");
                exit(1);
            }
            param__init(param);
            param->name = device_info->params[j].name;
            switch (device_info->params[j].type) {
                case INT:
                    param->val_case = PARAM__VAL_IVAL;
                    param->ival = param_data[j].p_i;
                    break;
                case FLOAT:
                    param->val_case = PARAM__VAL_FVAL;
                    param->fval = param_data[j].p_f;
                    break;
                case BOOL:
                    param->val_case = PARAM__VAL_BVAL;
                    param->bval = param_data[j].p_b;
                    break;
            }
            param->readonly = device_info->params[j].read && !device_info->params[j].write;
            device->params[device->n_params] = param;
            device->n_params++;
        }
        dev_idx++;
    }

    // Add custom log data to protobuf
    Device* custom = malloc(sizeof(Device));
    if (custom == NULL) {
        log_printf(FATAL, "send_device_data: Failed to malloc");
        exit(1);
    }
    device__init(custom);
    dev_data.devices[dev_idx] = custom;
    log_data_read(&num_params, custom_names, custom_types, custom_params);
    custom->n_params = num_params + 1;  // + 1 is for the current time
    custom->params = malloc(sizeof(Param*) * custom->n_params);
    if (custom->params == NULL) {
        log_printf(FATAL, "send_device_data: Failed to malloc");
        exit(1);
    }
    custom->name = "CustomData";
    custom->type = MAX_DEVICES;
    custom->uid = 2020;
    for (int i = 0; i < custom->n_params; i++) {
        Param* param = malloc(sizeof(Param));
        if (param == NULL) {
            log_printf(FATAL, "send_device_data: Failed to malloc");
            exit(1);
        }
        param__init(param);
        custom->params[i] = param;
        param->name = custom_names[i];
        switch (custom_types[i]) {
            case INT:
                param->val_case = PARAM__VAL_IVAL;
                param->ival = custom_params[i].p_i;
                break;
            case FLOAT:
                param->val_case = PARAM__VAL_FVAL;
                param->fval = custom_params[i].p_f;
                break;
            case BOOL:
                param->val_case = PARAM__VAL_BVAL;
                param->bval = custom_params[i].p_b;
                break;
        }
        param->readonly = true;  // CustomData is used to display changing values; Not an actual parameter
    }
    Param* time = malloc(sizeof(Param));
    if (time == NULL) {
        log_printf(FATAL, "send_device_data: Failed to malloc");
        exit(1);
    }
    param__init(time);
    custom->params[num_params] = time;
    time->name = "time_ms";
    time->val_case = PARAM__VAL_IVAL;
    time->ival = millis() - dawn_start_time;  // Can only give difference in millisecond since robot start since it is int32, not int64
    time->readonly = true;                    // Just displays the time; Writing to this parameter doesn't make sense

    dev_data.n_devices = dev_idx + 1;  // + 1 is for custom data

    len_pb = dev_data__get_packed_size(&dev_data);
    buffer = make_buf(DEVICE_DATA_MSG, len_pb);
    dev_data__pack(&dev_data, buffer + BUFFER_OFFSET);
	
	// acquire lock
	if (pthread_mutex_lock(conn_lock) != 0) {
		log_printf(ERROR, "send_device_data: failed to acquire lock: %s", strerror(errno));
	}

    //send message on socket
    if (writen(dawn_socket_fd, buffer, len_pb + BUFFER_OFFSET) == -1) {
        log_printf(ERROR, "send_device_data: sending log message over socket failed: %s", strerror(errno));
    }
	
	// release lock
	if (pthread_mutex_unlock(conn_lock) != 0) {
		log_printf(ERROR, "send_device_data: failed to release lock: %s", strerror(errno));
	}

    //free everything
    for (int i = 0; i < dev_data.n_devices; i++) {
        for (int j = 0; j < dev_data.devices[i]->n_params; j++) {
            free(dev_data.devices[i]->params[j]);
        }
        free(dev_data.devices[i]->params);
        free(dev_data.devices[i]);
    }
    free(dev_data.devices);
    free(buffer);  // Free buffer with device data protobuf
}

/*
 * Send a runtime status message to a client.
 * Arguments:
 *    - int conn_fd: socket connection's file descriptor on which to write to the TCP port
 *    - pthread_mutex_t* conn_lock: lock over the connection socket 
 */
void send_status_msg(int conn_fd, pthread_mutex_t* conn_lock) {
    RuntimeStatus status = RUNTIME_STATUS__INIT;
	robot_desc_val_t val;
    int len_pb;
    uint8_t* buffer;
	
	// populate the fields
	status.shep_connected = (robot_desc_read(SHEPHERD) == CONNECTED) ? 1 : 0;
	status.dawn_connected = (robot_desc_read(DAWN) == CONNECTED) ? 1 : 0;
	val = robot_desc_read(RUN_MODE);
	switch (val) {
		case IDLE:
			status.mode = MODE__IDLE;
			break;
		case AUTO:
			status.mode = MODE__AUTO;
			break;
		case TELEOP:
			status.mode = MODE__TELEOP;
			break;
		default:
			log_printf(ERROR, "send_status_msg: Unrecognized mode");
	}
	status.battery = 12.0; // change this when we can actually get the battery level
	status.version = malloc(strlen(RUNTIME_VERSION_STR) + 1);
	if (status.version == NULL) {
		log_printf(FATAL, "send_status_msg: Failed to malloc");
		return;
	}
	strcpy(status.version, RUNTIME_VERSION_STR);
	
    len_pb = runtime_status__get_packed_size(&status);
    buffer = make_buf(RUNTIME_STATUS_MSG, len_pb);
    runtime_status__pack(&status, buffer + BUFFER_OFFSET);

	// acquire lock
	if (pthread_mutex_lock(conn_lock) != 0) {
		log_printf(ERROR, "send_status_msg: failed to acquire lock: %s", strerror(errno));
	}

    // send message on socket
    if (writen(conn_fd, buffer, len_pb + BUFFER_OFFSET) == -1) {
        log_printf(ERROR, "send_status_msg: sending status message over socket failed: %s", strerror(errno));
    }
	
	// release lock
	if (pthread_mutex_unlock(conn_lock) != 0) {
		log_printf(ERROR, "send_status_msg: failed to acquire lock: %s", strerror(errno));
	}

    // free everything
	free(status.version); // free version string in status message
    free(buffer);  // free buffer with device data protobuf
}

// **************************************** RECEIVE MESSAGES ***************************************** //

/*
 * Processes new run mode message from client and reacts appropriately
 * Arguments:
 *    - uint8_t *buf: buffer containing packed protobuf with run mode message
 *    - uint16_t len_pb: length of buf
 *    - robot_desc_field_t client: DAWN or SHEPHERD, depending on which connection is being handled
 * Returns:
 *      0 on success (message was processed correctly)
 *     -1 on error unpacking message
 */
static int process_run_mode_msg(uint8_t* buf, uint16_t len_pb, robot_desc_field_t client) {
    RunMode* run_mode_msg = run_mode__unpack(NULL, len_pb, buf);
    if (run_mode_msg == NULL) {
        log_printf(ERROR, "recv_new_msg: Cannot unpack run_mode msg");
        return -1;
    }

    //if shepherd is connected and dawn tries to set RUN_MODE == AUTO or TELEOP, block it
    if (client == DAWN && robot_desc_read(SHEPHERD) == CONNECTED &&
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

    return 0;
}

/*
 * Processes new start position message from client and reacts appropriately
 * Arguments:
 *    - uint8_t *buf: buffer containing packed protobuf with run mode message
 *    - uint16_t len_pb: length of buf
 * Returns:
 *      0 on success (message was processed correctly)
 *     -1 on error unpacking message
 */
static int process_start_pos_msg(uint8_t* buf, uint16_t len_pb) {
    StartPos* start_pos_msg = start_pos__unpack(NULL, len_pb, buf);
    if (start_pos_msg == NULL) {
        log_printf(ERROR, "recv_new_msg: Cannot unpack start_pos msg");
        return -1;
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

    return 0;
}

/*
 * Processes new game state message from client and reacts appropriately
 * Arguments:
 *    - uint8_t *buf: buffer containing packed protobuf with run mode message
 *    - uint16_t len_pb: length of buf
 * Returns:
 *      0 on success (message was processed correctly)
 *     -1 on error unpacking message
 */
static int process_game_state_msg(uint8_t* buf, uint16_t len_pb) {
    GameState* game_state_msg = game_state__unpack(NULL, len_pb, buf);
    if (game_state_msg == NULL) {
        log_printf(ERROR, "recv_new_msg: Cannot unpack game_state msg");
        return -1;
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
    }
    game_state__free_unpacked(game_state_msg, NULL);

    return 0;
}

/*
 * Processes new time stamp message from client and reacts appropriately
 * Arguments:
 *    - int conn_fd: file descriptor of socket to send timestamp message back to Dawn
 *    - uint8_t *buf: buffer containing packed protobuf with run mode message
 *    - uint16_t len_pb: length of buf
 *    - pthread_mutex_t* conn_lock: lock over the connection socket 
 * Returns:
 *      0 on success (message was processed correctly)
 *     -1 on error unpacking message
 */
static int process_time_stamp_msg(int conn_fd, uint8_t* buf, uint16_t len_pb, pthread_mutex_t* conn_lock) {
    TimeStamps* time_stamp_msg = time_stamps__unpack(NULL, len_pb, buf);
    if (time_stamp_msg == NULL) {
        log_printf(ERROR, "recv_new_msg: Cannot unpack time_stamp msg");
        return -1;
    }
    send_timestamp_msg(conn_fd, time_stamp_msg, conn_lock);
    time_stamps__free_unpacked(time_stamp_msg, NULL);

    return 0;
}

/*
 * Processes new inputs message from client and reacts appropriately
 * Arguments:
 *    - uint8_t *buf: buffer containing packed protobuf with run mode message
 *    - uint16_t len_pb: length of buf
 * Returns:
 *      0 on success (message was processed correctly)
 *     -1 on error unpacking message
 */
static int process_inputs_msg(uint8_t* buf, uint16_t len_pb) {
    UserInputs* inputs = user_inputs__unpack(NULL, len_pb, buf);
    if (inputs == NULL) {
        log_printf(ERROR, "recv_new_msg: Failed to unpack UserInputs");
        return -1;
    }
    for (int i = 0; i < inputs->n_inputs; i++) {
        Input* input = inputs->inputs[i];
        // Convert Protobuf source enum to Runtime source enum
        robot_desc_field_t source = (input->source == SOURCE__GAMEPAD) ? GAMEPAD : KEYBOARD;
        robot_desc_write(source, input->connected ? CONNECTED : DISCONNECTED);
        if (input->connected) {
            if (source == KEYBOARD || (source == GAMEPAD && input->n_axes == 4)) {
                input_write(input->buttons, input->axes, source);
            } else {
                log_printf(ERROR, "recv_new_msg: Number of joystick axes given is %d which is not 4. Cannot update gamepad state", input->n_axes);
            }
        } else if (input->source == SOURCE__KEYBOARD) {
            log_printf(INFO, "recv_new_msg: Received keyboard disconnected from Dawn!!");
        }
    }
    user_inputs__free_unpacked(inputs, NULL);

    return 0;
}

/*
 * Receives new message from client on TCP connection and processes the message.
 * Arguments:
 *    - int conn_fd: socket connection's file descriptor from which to read the message
 *    - robot_desc_field_t client: DAWN or SHEPHERD, depending on which connection is being handled
 *    - pthread_mutex_t* conn_lock: lock over the connection socket
 * Returns: pointer to integer in which return status will be stored
 *     (int)msg_type where msg_type is the type of the received message on success
 *     -1 if message could not be parsed because client disconnected and connection closed
 *     -2 if message could not be unpacked or other error
 */
int recv_new_msg(int conn_fd, robot_desc_field_t client, pthread_mutex_t* conn_lock) {
    net_msg_t msg_type;  //message type
    uint16_t len_pb;     //length of incoming serialized protobuf message
    uint8_t* buf;        //buffer to read raw data into
    int ret = 0;         //return status OK by default

    int err = parse_msg(conn_fd, &msg_type, &len_pb, &buf);
    if (err == 0) {  // Means there is EOF while reading which means client disconnected
		free(buf);
        return -1;
    } else if (err == -1) {  // Means there is some other error while reading
		free(buf);
        return -2;
    }

    //unpack according to message
    switch (msg_type) {
        case RUN_MODE_MSG:
            if (process_run_mode_msg(buf, len_pb, client) != 0) {
                log_printf(ERROR, "recv_new_msg: error processing run mode");
                ret = -2;
            }
            break;
        case START_POS_MSG:
            if (process_start_pos_msg(buf, len_pb) != 0) {
                log_printf(ERROR, "recv_new_msg: error processing start position");
                ret = -2;
            }
            break;
        case GAME_STATE_MSG:
            if (process_game_state_msg(buf, len_pb) != 0) {
                log_printf(ERROR, "recv_new_msg: error processing game state");
                ret = -2;
            }
            break;
        case TIME_STAMP_MSG:
            if (process_time_stamp_msg(conn_fd, buf, len_pb, conn_lock) != 0) {
                log_printf(ERROR, "recv_new_msg: error processing time stamp");
                ret = -2;
            }
            break;
        case INPUTS_MSG:
            if (process_inputs_msg(buf, len_pb) != 0) {
                log_printf(ERROR, "recv_new_msg: error processing inputs");
                ret = -2;
            }
            break;
        default:
            log_printf(ERROR, "recv_new_msg: unknown message type %d", msg_type);
            ret = -2;
    }
    free(buf);
    if (ret == 0) {
    	return (int) msg_type;
    } else {
    	return ret;
    }
}
