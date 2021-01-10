#include "udp_conn.h"

pthread_t input_thread, device_thread;            // thread IDs for receiving gamepad data and sending device data
int socket_fd = -1;                               // the UDP socket's file descriptor
struct sockaddr_in dawn_addr = {0};               // the address of our client, which should be Dawn
socklen_t addr_len = sizeof(struct sockaddr_in);  // length of the address

uint64_t start_time;  // the time that Runtime/net_handler was started, in milliseconds since the UNIX epoch


static void* send_device_data(void* args) {
    int len;
    uint8_t* buffer;
    int err;
    // Needed to wait for UDP client to talk to us
    while (dawn_addr.sin_family == 0) {
        sleep(1);
    }

    uint32_t sub_map[MAX_DEVICES + 1];
    dev_id_t dev_ids[MAX_DEVICES];
    int valid_dev_idxs[MAX_DEVICES];
    uint32_t catalog;

    param_val_t custom_params[UCHAR_MAX];
    param_type_t custom_types[UCHAR_MAX];
    char custom_names[UCHAR_MAX][LOG_KEY_LENGTH];
    uint8_t num_params;

    while (1) {
        DevData dev_data = DEV_DATA__INIT;

        //get information
        get_catalog(&catalog);
        get_sub_requests(sub_map, NET_HANDLER);
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
            device_read_uid(device->uid, NET_HANDLER, DATA, sub_map[idx + 1], param_data);

            device->params = malloc(device_info->num_params * sizeof(Param*));
            if (device->params == NULL) {
                log_printf(FATAL, "send_device_data: Failed to malloc");
                exit(1);
            }
            //populate device parameters
            for (int j = 0; j < device_info->num_params; j++) {
                if (sub_map[idx + 1] & (1 << j)) {
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
                    device->params[device->n_params] = param;
                    device->n_params++;
                }
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
        time->ival = millis() - start_time;  // Can only give difference in millisecond since robot start since it is int32, not int64

        dev_data.n_devices = dev_idx + 1;  // + 1 is for custom data

        len = dev_data__get_packed_size(&dev_data);
        buffer = malloc(len);
        if (buffer == NULL) {
            log_printf(FATAL, "send_device_data: Failed to malloc");
            exit(1);
        }
        dev_data__pack(&dev_data, buffer);

        //send data to Dawn
        err = sendto(socket_fd, buffer, len, 0, (struct sockaddr*) &dawn_addr, addr_len);
        if (err < 0) {
            log_printf(ERROR, "send_device_data: UDP sendto failed: %s", strerror(errno));
        } else if (err != len) {
            log_printf(WARN, "send_device_data: Didn't send all data to Dawn UDP socket. send buffer length %d, actual sent %d", len, err);
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
        free(buffer);   // Free buffer with device data protobuf
        usleep(10000);  // Send data at 100 Hz
    }
    return NULL;
}


static void* update_inputs(void* args) {
    static int size = 128;  // No specific reason, just needs to be large enough to receive all UserInputs data
    uint8_t buffer[size];
    int recvlen;

    while (1) {
        recvlen = recvfrom(socket_fd, buffer, size, 0, (struct sockaddr*) &dawn_addr, &addr_len);
        // log_printf(DEBUG, "Dawn IP is %s:%d", inet_ntoa(dawn_addr.sin_addr), ntohs(dawn_addr.sin_port));
        if (recvlen == size) {
            log_printf(WARN, "update_inputs: UDP Read length matches read buffer size %d", recvlen);
        }
        if (recvlen < 0) {
            log_printf(ERROR, "update_inputs: UDP recvfrom failed: %s", strerror(errno));
            continue;
        }
        UserInputs* inputs = user_inputs__unpack(NULL, recvlen, buffer);
        if (inputs == NULL) {
            log_printf(ERROR, "update_inputs: Failed to unpack UserInputs");
            continue;
        }
        for (int i = 0; i < inputs->n_inputs; i++) {
            Input* input = inputs->inputs[i];
            // Convert Protobuf source enum to Runtime source enum
            robot_desc_field_t source = (input->source == SOURCE__GAMEPAD) ? GAMEPAD : KEYBOARD;
            robot_desc_write(source, input->connected ? CONNECTED : DISCONNECTED);
            if (input->connected) {
                if (source == GAMEPAD && input->n_axes != 4) {
                    log_printf(ERROR, "update_inputs: Number of joystick axes given is %d which is not 4. Cannot update gamepad state", input->n_axes);
                } else {
                    input_write(input->buttons, input->axes, source);
                }
            }
        }
        user_inputs__free_unpacked(inputs, NULL);
        memset(buffer, 0, size);
    }
    return NULL;
}


/******************************** PUBLIC FUNCTIONS *************************************/

void start_udp_conn() {
    //create the socket
    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        log_printf(ERROR, "start_udp_conn: could not create udp socket: %s", strerror(errno));
        return;
    }

    //bind the socket to any valid IP address on the raspi and specified port
    struct sockaddr_in my_addr = {0};  //for holding IP addresses (IPv4)
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(RASPI_UDP_PORT);
    if (bind(socket_fd, (struct sockaddr*) &my_addr, sizeof(struct sockaddr_in)) < 0) {
        log_printf(ERROR, "start_udp_conn: udp socket bind failed: %s", strerror(errno));
        return;
    }

    start_time = millis();

    //create threads
    if (pthread_create(&input_thread, NULL, update_inputs, NULL) != 0) {
        log_printf(ERROR, "start_udp_conn: failed to create update_gamepad_state thread: %s", strerror(errno));
        return;
    }
    if (pthread_create(&device_thread, NULL, send_device_data, NULL) != 0) {
        log_printf(ERROR, "start_udp_conn: failed to create send_device_data thread: %s", strerror(errno));
        stop_udp_conn();
    }
}

void stop_udp_conn() {
    if (pthread_cancel(input_thread) != 0) {
        log_printf(ERROR, "stop_udp_conn: failed to cancel input_thread: %s", strerror(errno));
    }
    if (pthread_cancel(device_thread) != 0) {
        log_printf(ERROR, "stop_udp_conn: failed to cancel device_thread: %s", strerror(errno));
    }
    if (pthread_join(input_thread, NULL) != 0) {
        log_printf(ERROR, "stop_udp_conn: failed to join input_thread: %s", strerror(errno));
    }
    if (pthread_join(device_thread, NULL) != 0) {
        log_printf(ERROR, "stop_udp_conn: failed to join device_thread: %s", strerror(errno));
    }
    if (close(socket_fd) != 0) {
        log_printf(ERROR, "stop_udp_conn: Couldn't close UDP socket properly: %s", strerror(errno));
    }
}
