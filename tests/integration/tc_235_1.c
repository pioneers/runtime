#include "../test.h"

#define DEVICE_NAME "GeneralTestDevice"
#define UID 1
#define PARAM_NAME "ALWAYS_TRUE"

/**
 * Tests #235 (should throw an error when trying to read a non-readable parameter)
 * Input: Not connected until after TELEOP
 * teleop_setup: Nothing
 * teleop_main: Only reads if 'w' is detected from the Keyboard
 * The Run Mode should also remain TELEOP throughout the whole test.
 */

int main() {
    start_test("Throw Device Error when reading a non-readable param", "tc_235_a", NO_REGEX);

    // Connect device and start TELEOP
    connect_virtual_device(DEVICE_NAME, UID);
    sleep(1);

    // Check that parameter is zero from the start
    param_val_t initial_param;
    initial_param.p_b = 1;
    same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, initial_param);

    // Connect Input, which should allow writes to happen when TELEOP starts
    float joysticks[4];
    send_user_input(get_button_bit("w"), joysticks, KEYBOARD);
    sleep(1);

    // Start TELEOP, which will attempt to write a nonzero value
    send_run_mode(SHEPHERD, TELEOP);
    sleep(1);

    // // Check that parameter is changed
    // param_val_t held_key_val;
    // held_key_val.p_i = 999;
    // same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, held_key_val);

    // // Disconnect input, which will stop robot and keep it to zero
    // // NOTE: If "MY_INT" isn't a parameter to kill, the value would be 111
    // disconnect_user_input();
    // sleep(1);
    // same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, zero);

    // // Reconnect input without pressing anything
    // send_user_input(0, joysticks, KEYBOARD);
    // sleep(1);

    // // Student code will write 111
    // param_val_t not_held_key_val;
    // not_held_key_val.p_i = 111;
    // same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, not_held_key_val);

    // // Press the key to write the other nonzero value
    // send_user_input(get_button_bit("w"), joysticks, KEYBOARD);
    // sleep(1);
    // same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, held_key_val);

    // // Check that the run mode remained TELEOP throughout the whole test
    // check_run_mode(TELEOP);
    return 0;
}
