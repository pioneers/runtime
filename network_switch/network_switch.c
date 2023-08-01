#include <logger.h>
#include <runtime_util.h>
#include <shm_wrapper.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>


#define SLEEP_DELAY 1
#define INITIAL_SWITCH 2

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
    int prev_switch = 2;

    char bash_exe[] = "/usr/bin/bash";
    char script_name[] = "network_switch.sh";

    char local_router[32];
    char local_password[32];
    char* local_args[5];
    char team_router[] = "teamrouter.txt";
    get_router_name_password(local_router, local_password, team_router);
    local_args[0] = bash_exe;
    local_args[1] = script_name;
    local_args[2] = local_router;
    local_args[3] = local_password;
    local_args[4] = NULL;

    char pie_router[32];
    char pie_password[32];
    char* pie_args[5];
    char pie_router_file[] = "pierouter.txt";
    get_router_name_password(pie_router, pie_password, pie_router_file);
    pie_args[0] = bash_exe;
    pie_args[1] = script_name;
    pie_args[2] = pie_router;
    pie_args[3] = pie_password;
    pie_args[4] = NULL;

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
                bool curr_switch_bool = param_data[device->num_params - 1].p_b;  // network switch value is the last parameter
                int curr_switch;
                if (curr_switch_bool) {
                    curr_switch = 1;
                } else if (!curr_switch_bool) {
                    curr_switch = 0;
                }
                while (curr_switch != prev_switch) {
                    int status;
                    if (curr_switch == 0) {  // switch to pioneers if curr_switch is false
                        pid_t pid;
                        if ((pid = fork()) < 0) {
                            log_printf(ERROR, "network_switch: Failed to fork");
                            sleep(SLEEP_DELAY);
                        } else if (pid == 0) {
                            execv(bash_exe, pie_args);  // call the bash script with pioneers router arguments
                        } else {
                            waitpid(pid, &status, 0);
                        }
                    } else {  // switch to student's router if curr_switch is true
                        pid_t pid;
                        if ((pid = fork()) < 0) {
                            log_printf(ERROR, "network_switch: Failed to fork");
                            sleep(SLEEP_DELAY);
                        } else if (pid == 0) {
                            execv(bash_exe, local_args);  // call the bash script with local router arguments
                        } else {
                            waitpid(pid, &status, 0);
                        }
                    }
                    char buf[5];
                    FILE* exit_status = fopen("exit_status.txt", "r");
                    fgets(buf, 5, exit_status);   // retrieve the output of the bash script in the exit_status.txt
                    if (strcmp(buf, "1") == 0) {  // if output is 1, we successfully connected. Set previous switch = current switch
                        log_printf(WARN, "SUCCESSFULLY CONNECTED TO NETWORK");
                        prev_switch = curr_switch;
                    } else {  // if output is not 1, we loop again to call the bash script again
                        log_printf(WARN, "FAILED TO CONNECT TO NETWORK");
                    }
                    fclose(exit_status);
                }
            }
        }
        sleep(SLEEP_DELAY);
    }
}
