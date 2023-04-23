#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../logger/logger.h"
#include "../shm_wrapper/shm_wrapper.h"
#include "../runtime_util/runtime_util.h"

int main() {
    while (1) {
        
    }
}

void check_net_switch() {
    /* pseudocode
    Receive data of all the devices connected
    Check if the device connected is PDB
    if device == PDB, check network_swtich. If network_swtich changed, call bash script
    */
	dev_id_t dev_ids[MAX_DEVICES];
	int valid_dev_idxs[MAX_DEVICES];
	uint32_t catalog;

	// get information
	get_catalog(&catalog);
	get_device_identifiers(dev_ids);

	//calculate num_devices, get valid device indices
	int num_devices = 0;
	for (int i = 0; i < MAX_dEVICES; i++) {
		if (catalog & (1 << i)) {
			valid_dev_idxs[num_devices] = i;
			num_devices++;
		} 
	}

}
/**
 * Sends a Device Data message to Dawn.
 * Arguments:
 *    - int dawn_socket_fd: socket fd for Dawn connection
 *    - uint64_t dawn_start_time: time that Dawn connection started, for calculating time in CustomData
 */
void send_device_data(int dawn_socket_fd, uint64_t dawn_start_time) {
    int len_pb;
    uint8_t* buffer;

    dev_id_t dev_ids[MAX_DEVICES];
    int valid_dev_idxs[MAX_DEVICES];
    uint32_t catalog;

    param_val_t custom_params[UCHAR_MAX];
    param_type_t custom_types[UCHAR_MAX];
    char custom_names[UCHAR_MAX][LOG_KEY_LENGTH];
    uint8_t num_params;

    DevData dev_data = DEV_DATA__INIT; // Unncessary. Used for protobuf

    // get information
    get_catalog(&catalog);
    get_device_identifiers(dev_ids);

    // calculate num_devices, get valid device indices
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

    // populate dev_data.device[i]
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
        // populate device parameters
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

    // send message on socket
    if (writen(dawn_socket_fd, buffer, len_pb + BUFFER_OFFSET) == -1) {
        log_printf(ERROR, "send_device_data: sending log message over socket failed: %s", strerror(errno));
    }

    // free everything
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

void handle_net_switch(relay_t* relay) {
    // static variable to keep track of previous state
    static bool current_switch = false;
    if (relay->dev_id.type == 7) {                                       // PDB type number is 7, look at runtime_util.c for addition info
        uint32_t pmap = 1 << 8;                                          // pmap set to only allow network switch value to be read since only bit 9 is 1, rest 0
        param_val_t* params = malloc(MAX_PARAMS * sizeof(param_val_t));  // Array of params to be filled on device_read()
        bool new_switch;
        if (params == NULL) {
            log_printf(FATAL, "handle_net_switch: Failed to malloc");
            exit(1);
        }
        // retrieve network switch from shared memory
        device_read(relay->shm_dev_idx, DEV_HANDLER, DATA, pmap, params);
        new_switch = params[8].p_b;
        // if the previous state is different from the current state
        // do the change_wifi.c code
        // set the current state to the previous state
        char* which_network[3];  // argv to pass into network_switch.c
        which_network[0] = "network_switch";
        which_network[2] = NULL;
        if (new_switch != current_switch) {
            pid_t pid;
            if ((pid = fork()) < 0) {  // fork to create a new process to run network_switch.c
                log_printf(WARN, "handle_net_switch: Failed to fork");
            } else if (pid == 0) {
                if (!new_switch) {
                    which_network[1] = "network 1";  // switch to pioneers if new_switch is false
                } else {
                    which_network[1] = "network 2";  // switch to students's router if new_switch is true
                }
                execv("network_switch", which_network);  // executes the executable with given file path and arguments
                log_printf(ERROR, "handle_net_switch: execv failed");

                exit(1);
            }
            current_switch = new_switch;
        }
    }
}

/* Old Network Switch Code
    if (strcmp(argv[1], "network 1") == 0) {
        system("sudo nmcli d wifi connect pioneers password pieisgreat");

    } else if (strcmp(argv[1], "network 2") == 0) {
        char* router_name = malloc(sizeof(char) * 32);
        char* router_password = malloc(sizeof(char) * 32);
        char* total_command = malloc(sizeof(char) * 128);
        FILE* team_router = fopen("../dev_handler/teamrouter.txt", "r");
        char curr_char;
        int curr_spot = 0;

        while ((curr_char = fgetc(team_router)) != '\n') {  // This while loop reads router name from teamrouter.txt
            router_name[curr_spot] = curr_char;
            curr_spot++;
        }
        router_name[curr_spot] = '\0';

        curr_spot = 0;
        for (int i = 0; i < 8; i++) {
            curr_char = fgetc(team_router);
            router_password[curr_spot] = curr_char;
            curr_spot++;
        }
        router_password[curr_spot] = '\0';

        sprintf(total_command, "sudo nmcli d wifi connect %s password %s", router_name, router_password);
        system(total_command);

        fclose(team_router);
        free(router_name);
        free(router_password);
        free(total_command);
    }
    return 0;
} */
