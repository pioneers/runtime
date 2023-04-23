#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../logger/logger.h"
#include "../shm_wrapper/shm_wrapper.h"
#include "../runtime_util/runtime_util.h"

#define SLEEP_DELAY 1

void get_router_name_password(char* router_name, char* router_password, char* router_txt) {
    FILE* router_file = fopen(router_txt, "r");
    fgets(router_name, 32, router_file);
    fgets(router_password, 32, router_file);
}

int main() {
    logger_init(NETWORK_SWITCH);
    shm_init();
    static bool prev_switch = false;

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

        // get information
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
        //check if device index is PDB 
        for (int i = 0; i < num_devices; i++) {
            int idx = valid_dev_idxs[i];
            if (dev_ids[idx].type == 7) {
                device_t* device = get_device(dev_ids[idx].type);
                param_val_t param_data[MAX_PARAMS];
                device_read(idx, NET_HANDLER, DATA, get_readable_param_bitmap(device -> type), param_data);
                bool curr_switch = param_data[device -> num_params - 1].p_b;
                if (curr_switch != prev_switch) {
                    if (!curr_switch) {//switch to pioneers if curr_switch is false
                        system("./network_switch.sh pie_router pie_password");
                    } else {//switch to student's router if curr_switch is true
                        system("./network_switch.sh local_router local_password");
                    }
                    curr_switch = prev_switch;
                }
            }
        }
        sleep(SLEEP_DELAY);
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
