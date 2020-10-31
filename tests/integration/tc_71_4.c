#include "../test.h"

/**
 * This test ensures that with no devices connected to the system, the
 * shared memory starts sending custom data back to Dawn as soon as the
 * first Gamepad State packet arrives on Runtime from Dawn.
 */

char check_output_1[] =
    "Current Robot Description:\n"
    "\tRUN_MODE = IDLE\n"
    "\tDAWN = CONNECTED\n"
    "\tSHEPHERD = CONNECTED\n"
    "\tGAMEPAD = CONNECTED\n"
    "\tSTART_POS = LEFT\n\n"
    "Current Gamepad State:\n"
    "\tPushed Buttons:\n"
    "\t\tbutton_a\n"
    "\t\tl_trigger\n"
    "\t\tdpad_down\n"
    "\tJoystick Positions:\n"
    "\t\tjoystick_left_x = -0.100000\n"
    "\t\tjoystick_left_y = 0.000000\n"
    "\t\tjoystick_right_x = 0.100000\n"
    "\t\tjoystick_right_y = 0.990000\n";

char check_output_2[] =
    "Device No. 0:\ttype = CustomData, uid = 2020, itype = 32\n";

int main() {
    // setup
    start_test("UDP; no devices connected");
    start_shm();
    start_net_handler();

    //poke system
    uint32_t buttons = (1 << BUTTON_A) | (1 << L_TRIGGER) | (1 << DPAD_DOWN);
    float joystick_vals[] = {-0.1, 0.0, 0.1, 0.99};
    send_gamepad_state(buttons, joystick_vals);
    sleep(1);  // Let gamepad state register
    print_shm();
    print_next_dev_data();

    // stop all processes
    stop_net_handler();
    stop_shm();
    end_test();

    // check outputs
    in_rest_of_output(check_output_1, NO_REGEX);
    in_rest_of_output(check_output_2, NO_REGEX);
    return 0;
}
