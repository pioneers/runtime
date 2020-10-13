/**
 * Acts as a substitute for net_handler_cli that starts with teleop mode
 * Sends corresonding gamepad state based on local machine keyboard inputs
 */
#include "../client/net_handler_client.h"

#include <stdio.h>
#include <arpa/inet.h>  // for inet_addr, bind, listen, accept, socket types
#include <netinet/in.h> // for structures relating to IPv4 addresses
#include <netdb.h>      // for struct addrinfo
#define PORT 5006 // This port must be exposed in docker-compose.yml
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
    joystick_right_y_up};
// ********************************** MAIN PROCESS ****************************************** //
void sigint_handler(int signum) {
    close_output();
    stop_net_handler();
    exit(0);
}

int connect_tcp(){
    int sockfd, connfd;
    struct sockaddr_in servaddr = {0};
    struct sockaddr_in cli = {0};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0){
        printf("socket bind failed... %s\n", strerror(errno));
        exit(0);
    }

    printf("Waiting for client\n");
    if((listen(sockfd, 5)) != 0){
        printf("listen failed...\n");
        exit(0);
    }
    socklen_t len = sizeof(cli);
    connfd = accept(sockfd, (struct sockaddr*)&cli, &len);
    printf("Accepted client\n");
    if (connfd < 0){
        printf("server accept failed...\n");
        exit(0);
    }
    return connfd;
}

int main() {
    signal(SIGINT, sigint_handler);

    // The bitmap of buttons pressed to be sent to net handler
    uint32_t buttons = 0;
    float joystick_vals[4] = {0};
    // The received bitstring from Python to be parsed into BUTTONS
    char buff[33];

    printf("Its gamer time\n");

    printf("Starting Net Handler CLI...\n");
    fflush(stdout);

    start_net_handler();
    sleep(2);
    // Connect dummy gamepad and start teleop mode
    send_gamepad_state(0, joystick_vals);
    send_run_mode(DAWN, TELEOP);
    sleep(2);

    // Start getting keyboard inputs
    int fd = connect_tcp();

    // Receive a bitstring of buttons and parse it
    while(1) {
        // Reset buttons/joysticks
        buttons = 0;
        memset(buff, 0, 32 * sizeof(char));
        memset(joystick_vals, 0, 4 * sizeof(float));
        // Read in bit string of size 32, not sizeof(buf)(33) since we are being sent a 32 len string
        recv(fd, buff, sizeof(buff) - 1, 0);
        buff[32] = '\0';

        // Parse joystick values
        for(int i = joystick_left_x_right; i <= joystick_right_y_up; i++){
            float pushed = 0;
            if (buff[i] == '1'){
                pushed = .25;
            }
            if (pushed != 0){
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

        // Parse the buttons TODO: EXTEND TO BUTTON_XBOX
        for(int i = button_a; i <= button_xbox; i++){
            if (buff[i] == '1'){
                buttons |= (1 << i);
            }
        }

        send_gamepad_state(buttons, joystick_vals);
        printf("SENT GAMEPAD STATE: %d\n", buttons);

    }
    return 0;
}
