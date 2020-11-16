#include "../test.h"

/**
 * This test ensures that with no devices connected to the system, the
 * shared memory starts sending custom data back to Dawn as soon as the
 * first Gamepad State packet arrives on Runtime from Dawn.
 */

#define ORDERED_STRINGS 1
#define UNORDERED_STRINGS 0

int main() {
    start_test("UDP; no devices connected", "", "", ORDERED_STRINGS, UNORDERED_STRINGS, NO_REGEX);

    // Send gamepad and check that the custom data is received
    uint32_t buttons = (1 << BUTTON_A) | (1 << L_TRIGGER) | (1 << DPAD_DOWN);
    float joystick_vals[] = {-0.1, 0.0, 0.1, 0.99};
    send_gamepad_state(buttons, joystick_vals);
    sleep(1);
    check_gamepad(buttons, joystick_vals);
    print_next_dev_data();
    add_ordered_string_output("Device No. 0:\ttype = CustomData, uid = 2020, itype = 32\n");

    end_test();
    return 0;
}
