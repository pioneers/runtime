#include "../test.h"

/**
 * This test ensures that with no devices connected to the system, the
 * shared memory starts sending custom data back to Dawn as soon as the
 * first Gamepad State packet arrives on Runtime from Dawn.
 */

char custom_data_output[] =
    "Device No. 0:\ttype = CustomData, uid = 0, itype = 32\n";

int main() {
    // setup
    start_test("UDP; no devices connected", "", "");

    // Send gamepad and check that the custom data is received
    uint32_t buttons = (1 << BUTTON_A) | (1 << L_TRIGGER) | (1 << DPAD_DOWN);
    float joystick_vals[] = {-0.1, 0.0, 0.1, 0.99};
    send_gamepad_state(buttons, joystick_vals);
    check_gamepad(buttons, joystick_vals);
    print_next_dev_data();

    // stop all processes
    end_test();

    // check outputs
    in_rest_of_output(custom_data_output);
    return 0;
}
