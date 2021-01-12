#include "../client/net_handler_client.h"

#define MAX_CMD_LEN 64  // maximum length of user input


// ********************************** COMMAND-SPECIFIC FUNCTIONS  ****************************** //

void prompt_run_mode() {
    robot_desc_field_t client = SHEPHERD;
    robot_desc_val_t mode = IDLE;
    char nextcmd[MAX_CMD_LEN];
    int keyboard_enabled = 0; // notify users when keyboard control is enabled/disabled
    // get client to send as
    while (1) {
        printf("Send as DAWN or SHEPHERD: ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        if (strcmp(nextcmd, "dawn\n") == 0) {
            client = DAWN;
            break;
        } else if (strcmp(nextcmd, "shepherd\n") == 0) {
            client = SHEPHERD;
            break;
        } else if (strcmp(nextcmd, "abort\n") == 0) {
            return;
        } else {
            printf("Invalid response to prompt: %s", nextcmd);
        }
    }

    // get mode to send
    while (1) {
        printf("Send mode IDLE, AUTO, or TELEOP: ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        if (strcmp(nextcmd, "idle\n") == 0) {
            mode = IDLE;
            keyboard_enabled = 0;
            break;
        } else if (strcmp(nextcmd, "auto\n") == 0) {
            mode = AUTO;
            keyboard_enabled = 0;
            break;
        } else if (strcmp(nextcmd, "teleop\n") == 0) {
            mode = TELEOP;
            keyboard_enabled = 1;
            break;
        } else if (strcmp(nextcmd, "abort\n") == 0) {
            return;
        } else {
            printf("Invalid response to prompt: %s", nextcmd);
        }
    }

    // send
    printf("Sending Run Mode message!\n");
    if(keyboard_enabled){
        printf("Keyboard controls now enabled!\n\n");
    }
    send_run_mode(client, mode);
}

void prompt_start_pos() {
    robot_desc_field_t client = SHEPHERD;
    robot_desc_val_t pos = LEFT;
    char nextcmd[MAX_CMD_LEN];

    // get client to send as
    while (1) {
        printf("Send as DAWN or SHEPHERD: ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        if (strcmp(nextcmd, "dawn\n") == 0) {
            client = DAWN;
            break;
        } else if (strcmp(nextcmd, "shepherd\n") == 0) {
            client = SHEPHERD;
            break;
        } else if (strcmp(nextcmd, "abort\n") == 0) {
            return;
        } else {
            printf("Invalid response to prompt: %s", nextcmd);
        }
    }

    // get pos to send
    while (1) {
        printf("Send pos LEFT or RIGHT: ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        if (strcmp(nextcmd, "left\n") == 0) {
            pos = LEFT;
            break;
        } else if (strcmp(nextcmd, "right\n") == 0) {
            pos = RIGHT;
            break;
        } else if (strcmp(nextcmd, "abort\n") == 0) {
            return;
        } else {
            printf("Invalid response to prompt: %s", nextcmd);
        }
    }

    // send
    printf("Sending Start Pos message!\n\n");
    send_start_pos(client, pos);
}

void prompt_challenge_data() {
    int client = SHEPHERD;
    char nextcmd[MAX_CMD_LEN];
    int MAX_CHALLENGES = 32;
    char** inputs = malloc(sizeof(char*) * MAX_CHALLENGES);
    if (inputs == NULL) {
        log_printf(FATAL, "prompt_challenge_data: Failed to malloc");
        exit(1);
    }

    // get client to send as
    while (1) {
        printf("Send as DAWN or SHEPHERD: ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        if (strcmp(nextcmd, "dawn\n") == 0) {
            client = DAWN;
            break;
        } else if (strcmp(nextcmd, "shepherd\n") == 0) {
            client = SHEPHERD;
            break;
        } else if (strcmp(nextcmd, "abort\n") == 0) {
            return;
        } else {
            printf("Invalid response to prompt: %s", nextcmd);
        }
    }

    // get challenge inputs to send
    int num_challenges;
    for (num_challenges = 0; num_challenges < MAX_CHALLENGES; num_challenges++) {
        // TODO: if we ever put the current names of challenges in runtime_util, make this print better
        printf("Provide input for challenge %d (or done or abort): ", num_challenges);
        fgets(nextcmd, MAX_CMD_LEN, stdin);

        if (strcmp(nextcmd, "done\n") == 0) {
            break;
        } else if (strcmp(nextcmd, "abort\n") == 0) {
            return;
        }

        // we need to do this because nextcmd has a newline at the end
        nextcmd[strlen(nextcmd) - 1] = '\0';
        inputs[num_challenges] = malloc(strlen(nextcmd) + 1);
        if (inputs[num_challenges] == NULL) {
            log_printf(FATAL, "prompt_challenge_data: Failed to malloc");
            exit(1);
        }
        strcpy(inputs[num_challenges], nextcmd);
    }

    // send
    printf("Sending Challenge Data message!\n\n");
    send_challenge_data(client, inputs, num_challenges);

    // free pointers
    for (int i = 0; i < num_challenges; i++) {
        free(inputs[i]);
    }
    free(inputs);
}

void prompt_device_data() {
    char nextcmd[MAX_CMD_LEN];
    char dev_names[DEVICES_LENGTH][32];  // for holding the names of the valid devices
    int num_devices = 0;
    uint8_t dev_type;
    long long int temp;
    dev_subs_t data[MAX_DEVICES];
    device_t* curr_dev;

    // first get the list of device names
    for (uint8_t i = 0; i < DEVICES_LENGTH; i++) {
        if ((curr_dev = get_device(i)) != NULL) {
            strcpy(dev_names[i], curr_dev->name);
        } else {
            strcpy(dev_names[i], "");
        }
    }

    // enter prompt loop
    while (1) {
        // ask for UID
        while (1) {
            printf("Enter the UID of the device, in base 10: ");
            fgets(nextcmd, MAX_CMD_LEN, stdin);

            if (strcmp(nextcmd, "abort\n") == 0) {
                return;
            }
            temp = strtoll(nextcmd, NULL, 10);
            if (temp < 0) {
                printf("Input is not a positive number: %lld\n", temp);
                continue;
            } else {
                data[num_devices].uid = (uint64_t) temp;
                break;
            }
        }

        // ask for device type
        data[num_devices].name = NULL;
        printf("Specify the device type, one of the following\n\t(type the number corresponding to the device type):\n");
        for (int i = 0; i < DEVICES_LENGTH; i++) {
            if (strcmp(dev_names[i], "") != 0) {
                printf("\t%4d %s\n", i, dev_names[i]);
            }
        }
        while (1) {
            printf("Enter the device type: ");
            fgets(nextcmd, MAX_CMD_LEN, stdin);
            if (strcmp(nextcmd, "abort\n") == 0) {
                return;
            }

            // Assign type and make sure it's an integer
            errno = 0;  // If strtol errors, errno will be set to a nonzero number. See the NOTES in https://man7.org/linux/man-pages/man3/strtol.3.html
            dev_type = (uint8_t) strtol(nextcmd, NULL, 10);
            if (errno != 0) {
                printf("Did not enter integer: %s\n", nextcmd);
                continue;
            }

            // Make sure the type refers to a valid device
            curr_dev = get_device(dev_type);
            if (curr_dev == NULL) {
                printf("Did not specify a valid device type: %s\n", nextcmd);
                continue;
            }

            // Assign name to data
            data[num_devices].name = malloc(strlen(curr_dev->name) + 1);
            strcpy(data[num_devices].name, curr_dev->name);
            break;
        }

        // fetch parameters and prompt user "y" or "n" on each one
        data[num_devices].params = 0;
        printf("Now specify whether to subscribe to given param or not.\n");
        printf("If a subscription is desired, type lowercase letter \"y\". Otherwise, press anything else: \n");
        for (int i = 0; i < curr_dev->num_params; i++) {
            printf("\t%s? ", curr_dev->params[i].name);
            fgets(nextcmd, MAX_CMD_LEN, stdin);
            // Abort subscription completely if "abort"
            if (strcmp(nextcmd, "abort\n") == 0) {
                return;
            }
            // Add subscription if "y"
            if (strcmp(nextcmd, "y\n") == 0) {
                data[num_devices].params |= (1 << i);
            }
        }
        num_devices++;

        // subscribe to another device?
        printf("Type \"next\" to add another device subscription. If ready to send queued subscriptions, type anything else: \n");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        if (strcmp(nextcmd, "next\n") != 0) {
            break;
        }
    }

    // send
    printf("Sending Device Data message!\n\n");
    send_device_subs(data, num_devices);

    // free everything
    for (int i = 0; i < num_devices; i++) {
        free(data[i].name);
    }
}

void prompt_reroute_output() {
    char* dest = "net_handler_output.log";
    char nextcmd[MAX_CMD_LEN];

    printf("Enter new output destination (blank for net_handler_output.log): ");
    fgets(nextcmd, MAX_CMD_LEN, stdin);
    if (strcmp(nextcmd, "\n") != 0) {
        // truncate new line character
        nextcmd[strcspn(nextcmd, "\n")] = 0;
        dest = nextcmd;
    }

    update_tcp_output_fp(dest);
}

void display_help() {
    printf("This is the main menu. Type one of the following commands to send a message to net_handler.\n");
    printf("Once you type one of the commands and hit \"enter\", follow on-screen instructions.\n");
    printf("At any point while following the instructions, type \"abort\" to go back to this main menu.\n");
    printf("All commands should be typed in all lower case (including when being prompted to send messages)\n");
    printf("\trun mode           send a Run Mode message\n");
    printf("\tstart pos          send a Start Pos message\n");
    printf("\tchallenge data     send a Challenge Data message\n");
    printf("\tdevice data        send a Device Data message (send a subscription request)\n");
    printf("\tview device data   view the next UDP packet sent to Dawn containing most recent device data\n");
    printf("\treroute output     reroute output to a file\n");
    printf("\thelp               display this help text\n");
    printf("\texit               exit the Net Handler CLI\n");
}

// ********************************** MAIN PROCESS ****************************************** //

void sigint_handler(int signum) {
    close_output();
    exit(0);
}

int main() {
    char nextcmd[MAX_CMD_LEN];  // to hold nextline
    int stop = 0;

    signal(SIGINT, sigint_handler);

    printf("Starting Net Handler CLI...\n");
    fflush(stdout);

    // start the net handler and connect all of its output locations to file descriptors in this process
    start_net_handler();

    // command-line loop which prompts user for commands to send to net_handler
    while (!stop) {
        // get the next command
        printf("> ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);

        // compare input string against the available commands
        if (strcmp(nextcmd, "exit\n") == 0) {
            stop = 1;
        } else if (strcmp(nextcmd, "reroute output\n") == 0) {
            prompt_reroute_output();
        } else if (strcmp(nextcmd, "run mode\n") == 0) {
            prompt_run_mode();
        } else if (strcmp(nextcmd, "start pos\n") == 0) {
            prompt_start_pos();
        } else if (strcmp(nextcmd, "challenge data\n") == 0) {
            prompt_challenge_data();
        } else if (strcmp(nextcmd, "device data\n") == 0) {
            prompt_device_data();
        } else if (strcmp(nextcmd, "view device data\n") == 0) {
            print_next_dev_data();
        } else if (strcmp(nextcmd, "help\n") == 0) {
            display_help();
        } else {
            printf("Invalid command %s", nextcmd);
            display_help();
        }
        usleep(500000);
    }

    printf("Exiting Net Handler CLI...\n");

    stop_net_handler();

    printf("Done!\n");

    return 0;
}
