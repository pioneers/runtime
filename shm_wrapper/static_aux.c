#include <signal.h>
#include "shm_wrapper.h"

void ctrl_c_handler(int sig_num) {
    printf("Aborting and cleaning up\n");
    fflush(stdout);
    exit(0);
}

int main() {
    logger_init(NET_HANDLER);
    shm_init();
    signal(SIGINT, ctrl_c_handler);
    robot_desc_write(GAMEPAD, CONNECTED);
    robot_desc_write(RUN_MODE, AUTO);
    print_robot_desc();

    uint32_t buttons;
    float joystick_vals[4];

    buttons = 34788247; // push some random buttons
    joystick_vals[X_LEFT_JOYSTICK] = -0.4854;
    joystick_vals[Y_LEFT_JOYSTICK] = 0.58989;
    joystick_vals[X_RIGHT_JOYSTICK] = 0.9898;
    joystick_vals[Y_RIGHT_JOYSTICK] = -0.776;
	
    gamepad_write(buttons, joystick_vals);
    print_gamepad_state();

    sleep(5);
    robot_desc_write(RUN_MODE, TELEOP);

    int size = 30;
    char mode[size];
    while(1) {
        fgets(mode, size, stdin);
        if (strcmp(mode, "auto\n") == 0) {
            robot_desc_write(RUN_MODE, AUTO);
        }
        else if (strcmp(mode, "teleop\n") == 0) {
            robot_desc_write(RUN_MODE, TELEOP);
        }
        else if (strcmp(mode, "idle\n") == 0) {
            robot_desc_write(RUN_MODE, IDLE);
        }
        sleep(1);
    }
}