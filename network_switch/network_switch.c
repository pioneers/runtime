#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../logger/logger.h"
#include "../runtime_util/runtime_util.h"
#include "../shm_wrapper/shm_wrapper.h"

#define SLEEP_DELAY 1

// Retrieves router name and password from the file name: router_txt
void get_router_name_password(char* router_name, char* router_password, char* router_txt) {
    FILE* router_file = fopen(router_txt, "r");
    fgets(router_name, 32, router_file);
    router_name[strlen(router_name) - 1] = '\0';
    fgets(router_password, 32, router_file);
    router_password[strlen(router_password) - 1] = '\0';
    fclose(router_file);
}

int main() {
    logger_init(NETWORK_SWITCH);
    shm_init();
    chdir("../network_switch");
    log_printf(INFO, "NETWORK_SWITCH initialized.");
    bool prev_switch = false;

    char local_router[32];
    char local_password[32];
    char team_router[] = "teamrouter.txt";
    get_router_name_password(local_router, local_password, team_router);
    char pie_router[32];
    char pie_password[32];
    char pie_router_file[] = "pierouter.txt";
    get_router_name_password(pie_router, pie_password, pie_router_file);

    while (1) {
        dev_id_t dev_ids[MAX_DEVICES];
        int valid_dev_idxs[MAX_DEVICES];
        uint32_t catalog;
        char total_command[128];

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
        // check if device index is PDB
        for (int i = 0; i < num_devices; i++) {
            int idx = valid_dev_idxs[i];
            if (dev_ids[idx].type == 7) {
                device_t* device = get_device(dev_ids[idx].type);
                param_val_t param_data[MAX_PARAMS];
                device_read(idx, NET_HANDLER, DATA, get_readable_param_bitmap(device->type), param_data);
                bool curr_switch = param_data[device->num_params - 1].p_b;  // network switch value is the last parameter
                if (curr_switch != prev_switch) {
                    if (!curr_switch) {  // switch to pioneers if curr_switch is false
                        snprintf(total_command, sizeof(total_command), "./network_switch.sh %s %s", pie_router, pie_password);
                        system(total_command);
                    } else {  // switch to student's router if curr_switch is true
                        snprintf(total_command, sizeof(total_command), "./network_switch.sh %s %s", local_router, local_password);
                        system(total_command);
                    }
                    prev_switch = curr_switch;
                }
            }
        }
        sleep(SLEEP_DELAY);
    }
}