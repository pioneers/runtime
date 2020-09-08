#include "../client/net_handler_client.h"

#include <stdio.h>

char *file = "gamepad_inputs.fifo";
FILE *fd;
char character;
u_int32_t buttons = 0;
float joystick_vals[4];
void set_state(int set);
// ********************************** MAIN PROCESS ****************************************** //
void sigint_handler(int signum) {
    close_output();
    exit(0);
}

int main() {

    signal(SIGINT, sigint_handler);

    fd = fopen(file, "r");
    if(fd == NULL){
        printf("not opening file correctly");
        stop_net_handler();
        close_output();
        exit(0);
    }

    printf("Its gamer time...\n");
    fflush(stdout);

    // start the net handler and connect all of its output locations to file descriptors in this process
    start_net_handler();
    while(1){
        printf("Getting character");
        int set = -1;
        character = fgetc(fd);
        if(character == EOF){
            break;
        }
        if(character == 'u'){
            set = 0;
        }
        else if(character == 'i'){
            set = 1;
        }
        else if(character == 'o'){
            set = 2;
        }
        else if(character == 'p'){
            set = 3;
        }
        else if(character == 'h'){
            set = 4;
        }
        else if(character =='j'){
            set = 5;
        }
        else if(character == 'k'){
            set = 6;
        }
        else if(character == 'l'){
            set = 7;
        }
        else if(character == 'f'){
            set = 8;
        }
        else if(character == 'g'){
            set = 9;
        }
        else if(character == 'r'){
            set = 10;
        }
        else if(character == 't'){
            set = 11;
        }
        else if(character == 'w'){
            set = 12;
        }
        else if(character == 's'){
            set = 13;
        }
        else if(character == 'a'){
            set = 14;
        }
        else if(character == 'd'){
            set = 15;
        }
        else if(character == 'z'){
            set = 16;
        }
        else if (character == ':'){

            character = fgetc(fd);

            if(character == 'B'){
                joystick_vals[0] = 1;
            }
            else if(character == 'N'){
                joystick_vals[0] = -1;
            }
            else if(character == 'V'){
                joystick_vals[1] = 1;
            }
            else if(character == 'M'){
                joystick_vals[1] = -1;
            }
        }
        if(set != -1){
            set_state(set);
        }
        printf("the button pressed is %d \n", set);
        send_gamepad_state(buttons, joystick_vals);
    }
    return 0;
}
void set_state(int set){
        buttons |= (1 << set);
    }