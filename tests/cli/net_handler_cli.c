#include "../client/net_handler_client.h"
#include "keyboard_interface.h"

#define MAX_CMD_LEN 64  // maximum length of user input

/**
 * If the CLI is ran without any flags, it will spawn a new instance of net handler.
 * This may not be desired of an instance of net handler is already running.
 * The CLI supports two flags. If either are specified, the CLI will "attach" to the existing
 * instance of net handler.
 * Flags:
 *   -d: Connects a fake Dawn
 *   -s: Connects a fake Shepherd
 * 
 * If no flags are specified, both (fake) Dawn and (fake) Shepherd will be connected to a new
 * net handler instance.
 */
bool spawned_net_handler = true;  // Whether the CLI spawned a new net handler instance
bool dawn = false;                // Whether to connect a fake Dawn
bool shepherd = false;            // Whether to connect a fake Shepherd

// ********************************** COMMAND-SPECIFIC FUNCTIONS  ****************************** //

void prompt_run_mode() {
    robot_desc_field_t client = SHEPHERD;
    robot_desc_val_t mode = IDLE;
    char nextcmd[MAX_CMD_LEN];
    bool keyboard_enabled = false;  // notify users when keyboard control is enabled/disabled
    // get client to send as
    while (true) {
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
    while (true) {
        printf("Send mode IDLE, AUTO, or TELEOP: ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        if (strcmp(nextcmd, "idle\n") == 0) {
            mode = IDLE;
            keyboard_enabled = false;
            break;
        } else if (strcmp(nextcmd, "auto\n") == 0) {
            mode = AUTO;
            keyboard_enabled = false;
            break;
        } else if (strcmp(nextcmd, "teleop\n") == 0) {
            mode = TELEOP;
            keyboard_enabled = true;
            break;
        } else if (strcmp(nextcmd, "abort\n") == 0) {
            return;
        } else {
            printf("Invalid response to prompt: %s", nextcmd);
        }
    }

    // send
    printf("Sending Run Mode message!\n");
    if (keyboard_enabled) {
        printf("Keyboard controls now enabled!\n\n");
    }
    send_run_mode(client, mode);
}

void prompt_game_state() {
    robot_desc_field_t state;
    char nextcmd[MAX_CMD_LEN];
    //get state to send
    while (true) {
        printf("Send state POISON_IVY, DEHYDRATION, HYPOTHERMIA: ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        if (strcmp(nextcmd, "poison ivy\n") == 0) {
            state = POISON_IVY;
            break;
        } else if (strcmp(nextcmd, "dehydration\n") == 0) {
            state = DEHYDRATION;
            break;
        } else if (strcmp(nextcmd, "hypothermia\n") == 0) {
            state = HYPOTHERMIA;
            break;
        } else if (strcmp(nextcmd, "abort\n") == 0) {
            return;
        } else {
            printf("Invalid response to prompt: %s", nextcmd);
        }
    }
    printf("Sending Game State message!\n");
    send_game_state(state);
}

void prompt_start_pos() {
    robot_desc_field_t client = SHEPHERD;
    robot_desc_val_t pos = LEFT;
    char nextcmd[MAX_CMD_LEN];

    // get client to send as
    while (true) {
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
    while (true) {
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

void print_next_dev_data() {
    DevData* dev_data = get_next_dev_data();
    print_dev_data(stdout, dev_data);
    dev_data__free_unpacked(dev_data, NULL);
}

void display_help() {
    printf("This is the main menu. Type one of the following commands to send a message to net_handler.\n");
    printf("Once you type one of the commands and hit \"enter\", follow on-screen instructions.\n");
    printf("At any point while following the instructions, type \"abort\" to go back to this main menu.\n");
    printf("All commands should be typed in all lower case (including when being prompted to send messages)\n");
    printf("\trun mode           send a Run Mode message\n");
    printf("\tgame state         send a Game State message\n");
    printf("\tstart pos          send a Start Pos message\n");

    printf("\tsend timestamp     send a timestamp message to Dawn to test latency\n");
    printf("\tview device data   view the next Device Data message sent to Dawn containing most recent device data\n");
    printf("\thelp               display this help text\n");
    printf("\texit               exit the Net Handler CLI\n");
}

// ********************************** MAIN PROCESS ****************************************** //

void sigint_handler(int signum) {
    close_output();
    exit(0);
}

void connect_keyboard() {
    pthread_t keyboard_id;  // id of thread running the keyboard_interface
    if (pthread_create(&keyboard_id, NULL, (void*) setup_keyboard, NULL) != 0) {
        printf("pthread create: failed to create keyboard thread: %s", strerror(errno));
    }
}

int main(int argc, char** argv) {
    char nextcmd[MAX_CMD_LEN];  // to hold nextline
    bool stop = false;

    signal(SIGINT, sigint_handler);

    // Parse command line flags
    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "ds")) != -1) {
        switch (c) {
            case 'd':
                spawned_net_handler = false;
                dawn = true;
                break;
            case 's':
                spawned_net_handler = false;
                shepherd = true;
                break;
            case '?':
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                return 1;
            default:
                return 1;
        }
    }

    printf("Starting Net Handler CLI...\n");
    fflush(stdout);

    // Handle behavior based on flags
    if (spawned_net_handler) {
        // Since we spawn a new net handler, we should also connect a fake Dawn and fake Shepherd
        dawn = true;
        shepherd = true;
        // start the net handler and connect all of its output locations to file descriptors in this process
        start_net_handler();
    } else {  // In "attach-only" mode
        connect_clients(dawn, shepherd);
        if (dawn) {
            // execute the keyboard_interface on a seperate thread
            connect_keyboard();
            sleep(1);
        }
    }

    // command-line loop which prompts user for commands to send to net_handler
    while (!stop) {
        // get the next command
        printf("> ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);

        // compare input string against the available commands
        if (strcmp(nextcmd, "exit\n") == 0) {
            stop = true;
        } else if (strcmp(nextcmd, "run mode\n") == 0) {
            prompt_run_mode();
        } else if (strcmp(nextcmd, "start pos\n") == 0) {
            prompt_start_pos();
        } else if (strcmp(nextcmd, "game state\n") == 0) {
            prompt_game_state();
        } else if (strcmp(nextcmd, "view device data\n") == 0) {
            print_next_dev_data();
        } else if (strcmp(nextcmd, "send timestamp\n") == 0) {
            send_timestamp();
        } else if (strcmp(nextcmd, "help\n") == 0) {
            display_help();
        } else {
            printf("Invalid command %s", nextcmd);
            display_help();
        }
        usleep(500000);
    }

    printf("Exiting Net Handler CLI...\n");

    if (spawned_net_handler) {
        stop_net_handler();
    }
    return 0;
}
