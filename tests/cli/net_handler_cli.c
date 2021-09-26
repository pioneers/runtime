#include <ctype.h>
#include "../client/net_handler_client.h"

#define MAX_CMD_LEN 64  // maximum length of user input
#define NUM_GAMEPAD_BUTTONS_AND_JOYSTICKS (NUM_GAMEPAD_BUTTONS + 2 * NUM_GAMEPAD_JOYSTICKS)
#define KEYBOARD_PORT 5006  // If running on docker, this port must be exposed in docker-compose.yml

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

// ***************************** KEYBOARD SPECIFIC ************************** //

// Order of buttons we receive from the Python script
enum buttons {
    button_a,
    button_b,
    button_x,
    button_y,
    l_bumper,
    r_bumper,
    l_trigger,
    r_trigger,
    button_back,
    button_start,
    l_stick,
    r_stick,
    dpad_up,
    dpad_down,
    dpad_left,
    dpad_right,
    button_xbox,
    joystick_left_x_right,
    joystick_left_x_left,
    joystick_left_y_down,
    joystick_left_y_up,
    joystick_right_x_left,
    joystick_right_x_right,
    joystick_right_y_down,
    joystick_right_y_up
};

int connect_keyboard_tcp() {
    int sockfd, connfd;
    struct sockaddr_in servaddr = {0};
    struct sockaddr_in cli = {0};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("connect_keyboard_tcp: socket creation failed...\n");
        exit(0);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(KEYBOARD_PORT);

    if ((bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr))) != 0) {
        printf("connect_keyboard_tcp: socket bind failed... %s\n", strerror(errno));
        exit(0);
    }

    if ((listen(sockfd, 5)) != 0) {
        printf("connect_keyboard_tcp: listen failed...\n");
        exit(0);
    }
    socklen_t len = sizeof(cli);
    connfd = accept(sockfd, (struct sockaddr*) &cli, &len);
    if (connfd < 0) {
        printf("connect_keyboard_tcp: server accept failed...\n");
        exit(0);
    }
    return connfd;
}

void setup_keyboard() {
    // The bitmap of buttons pressed to be sent to net handler
    uint64_t gamepad_buttons = 0;
    float joystick_vals[4] = {0};
    // The received gamepad bitstring from Python to be parsed into BUTTONS
    char gamepad_buff[NUM_GAMEPAD_BUTTONS_AND_JOYSTICKS + 1];

    uint64_t keyboard_buttons = 0;
    // The received keyboard bitstring from Python
    char keyboard_buff[NUM_KEYBOARD_BUTTONS + 1];

    // Start getting keyboard inputs
    int fd = connect_keyboard_tcp();

    printf("Keyboard waiting for user input\n");

    // Receive a bitstring of buttons and parse it
    while (1) {
        // Reset buttons/joysticks
        gamepad_buttons = 0;
        memset(gamepad_buff, 0, NUM_GAMEPAD_BUTTONS_AND_JOYSTICKS * sizeof(char));
        memset(joystick_vals, 0, NUM_GAMEPAD_JOYSTICKS * sizeof(float));
        // Read in bit string of size NUM_GAMEPAD_BUTTONS_AND_JOYSTICKS, not sizeof(gamepad_buf)(NUM_GAMEPAD_BUTTONS_AND_JOYSTICKS + 1) since we are being sent a NUM_GAMEPAD_BUTTONS_AND_JOYSTICKS len string
        recv(fd, gamepad_buff, NUM_GAMEPAD_BUTTONS_AND_JOYSTICKS, 0);
        gamepad_buff[NUM_GAMEPAD_BUTTONS_AND_JOYSTICKS] = '\0';

        // Reset keyboard
        keyboard_buttons = 0;
        memset(keyboard_buff, 0, NUM_KEYBOARD_BUTTONS * sizeof(char));
        recv(fd, keyboard_buff, NUM_KEYBOARD_BUTTONS, 0);
        keyboard_buff[NUM_KEYBOARD_BUTTONS] = '\0';
        // Parse joystick values
        for (int i = joystick_left_x_right; i <= joystick_right_y_up; i++) {
            float pushed = 0;
            if (gamepad_buff[i] == '1') {
                pushed = .25;
            }
            if (pushed != 0) {
                switch (i) {
                    case joystick_left_x_right:
                        joystick_vals[0] = pushed;
                        break;
                    case joystick_left_x_left:
                        joystick_vals[0] = -1 * pushed;
                        break;
                    case joystick_left_y_down:
                        joystick_vals[1] = -1 * pushed;
                        break;
                    case joystick_left_y_up:
                        joystick_vals[1] = 1 * pushed;
                        break;
                    case joystick_right_x_left:
                        joystick_vals[2] = -1 * pushed;
                        break;
                    case joystick_right_x_right:
                        joystick_vals[2] = 1 * pushed;
                        break;
                    case joystick_right_y_down:
                        joystick_vals[3] = -1 * pushed;
                        break;
                    case joystick_right_y_up:
                        joystick_vals[3] = pushed;
                        break;
                }
            }
        }

        // Set bitmap for gamepad
        for (int i = 0; i < NUM_GAMEPAD_BUTTONS; i++) {
            if (gamepad_buff[i] == '1') {
                gamepad_buttons |= (1 << i);
            }
        }

        // Set bitmap for keyboard
        for (int i = 0; i < NUM_KEYBOARD_BUTTONS; i++) {
            if (keyboard_buff[i] == '1') {
                keyboard_buttons |= (((uint64_t) 1) << i);
            }
        }

        send_user_input(gamepad_buttons, joystick_vals, GAMEPAD);
        send_user_input(keyboard_buttons, joystick_vals, KEYBOARD);
    }
}


// ********************************** COMMAND-SPECIFIC FUNCTIONS  ****************************** //

void lowercase_string(char* str) {
    for(int i = 0; str[i] != '\0'; i++){
        str[i] = tolower(str[i]);
    }
}

void prompt_run_mode() {
    robot_desc_field_t client = dawn ? DAWN : SHEPHERD;
    robot_desc_val_t mode = IDLE;
    char nextcmd[MAX_CMD_LEN];
    bool keyboard_enabled = false;  // notify users when keyboard control is enabled/disabled

    // get client only if both clients are enabled
    while (dawn && shepherd) {
        printf("Send as DAWN or SHEPHERD: ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        lowercase_string(nextcmd);
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
        lowercase_string(nextcmd);
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
        lowercase_string(nextcmd);
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
    robot_desc_field_t client = dawn ? DAWN : SHEPHERD;
    robot_desc_val_t pos = LEFT;
    char nextcmd[MAX_CMD_LEN];

    // get client only if both clients are enabled
    while (dawn && shepherd) {
        printf("Send as DAWN or SHEPHERD: ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        lowercase_string(nextcmd);
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
        lowercase_string(nextcmd);
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
    if (dev_data == NULL) {
        printf("WARN: Haven't received any device data from Runtime yet\n");
        return;
    }
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
    printf("\tdevice data        view the next Device Data message sent to Dawn containing most recent device data\n");
    printf("\thelp               display this help text\n");
    printf("\texit               exit the Net Handler CLI\n");
}

// ********************************** MAIN PROCESS ****************************************** //

void sigint_handler(int signum) {
    stop_net_handler();
    exit(0);
}

int main(int argc, char** argv) {
    char nextcmd[MAX_CMD_LEN];  // to hold nextline
    bool stop = false;

    signal(SIGINT, sigint_handler);

    // Parse command line flags
    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "dsh")) != -1) {
        switch (c) {
            case 'd':
                spawned_net_handler = false;
                dawn = true;
                break;
            case 's':
                spawned_net_handler = false;
                shepherd = true;
                break;
            case 'h':
                printf(
                    "Net Handler CLI Help:\n\n"
                    "Act like Dawn or Shepherd and send/receive commands to/from Runtime's Network Handler\n\n"
                    "\t-d: Attach to net_handler acting like Dawn\n"
                    "\t-s: Attack to net_handler acting like Shepherd\n"
                    "\t-h: Display this help message\n"
                    "\tDefault: Start net_handler and attach with both Dawn and Shepherd\n"
                );
                return 0;
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
    }

    if (dawn) {
        // execute the keyboard interface on a seperate thread
        pthread_t keyboard_id;
        if (pthread_create(&keyboard_id, NULL, (void*) setup_keyboard, NULL) != 0) {
            printf("pthread create: failed to create keyboard thread: %s", strerror(errno));
            exit(1);
        }
        sleep(1);
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
        } else if (strcmp(nextcmd, "device data\n") == 0) {
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
