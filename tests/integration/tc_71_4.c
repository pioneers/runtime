#include "../test.h"

/**
 * This test ensures that with no devices connected to the system, the
 * shared memory starts sending custom data back to Dawn as soon as the
 * first Gamepad State packet arrives on Runtime from Dawn.
 */

int main() {
    start_test("UDP; no devices connected", "", "", NO_REGEX);

    // Send gamepad and check that the custom data is received
    uint64_t buttons = get_button_bit("button_a") | get_button_bit("l_trigger") | get_button_bit("dpad_down");
    float joystick_vals[] = {-0.1, 0.0, 0.1, 0.99};
    send_user_input(buttons, joystick_vals, GAMEPAD);
    sleep(1);
    send_user_input(buttons, joystick_vals, GAMEPAD);
    print_next_dev_data();
    add_ordered_string_output("Device No. 0:\ttype = CustomData, uid = 2020, itype = 32\n");

    end_test();
    return 0;
}
