/**
 * Sends corresonding gamepad state based on local machine keyboard inputs
 */
#include "keyboard_interface.h"

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

// ********************************** HELPER FUNCTIONS ****************************************** //

int connect_tcp() {
    int sockfd, connfd;
    struct sockaddr_in servaddr = {0};
    struct sockaddr_in cli = {0};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("connect_tcp: socket creation failed...\n");
        exit(0);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if ((bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr))) != 0) {
        printf("connect_tcp: socket bind failed... %s\n", strerror(errno));
        exit(0);
    }

    printf("connect_tcp: Keyboard waiting for client\n");

    if ((listen(sockfd, 5)) != 0) {
        printf("connect_tcp: listen failed...\n");
        exit(0);
    }
    socklen_t len = sizeof(cli);
    connfd = accept(sockfd, (struct sockaddr*) &cli, &len);
    printf("connect_tcp: Accepted client\n");
    if (connfd < 0) {
        printf("connect_tcp: server accept failed...\n");
        exit(0);
    }
    return connfd;
}

// ********************************** MAIN PROCESS ****************************************** //

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
    printf("setup_keyboard: Connecting keyboard\n");
    int fd = connect_tcp();
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
                keyboard_buttons |= (1LL << i);
            }
        }

        send_user_input(gamepad_buttons, joystick_vals, GAMEPAD);
        send_user_input(keyboard_buttons, joystick_vals, KEYBOARD);
    }
}
