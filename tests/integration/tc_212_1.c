#include "../test.h"

#define DEVICE_NAME "SimpleTestDevice"
#define UID 1
#define PARAM_NAME "MY_INT"

/**
 * Tests #212 (parameters that need to be emergency stopped)
 * Input: Not connected until after TELEOP
 * teleop_setup: Nothing
 * teleop_main: Always writes
 * If no UserInput is connected, writing nonzero values to emergency stopped parameters will be void
 * until a UserInput is connected.
 * The Run Mode should also remain TELEOP throughout the whole test.
 */

int main() {
    start_test("Void Writes Until Input Connected", "tc_212_a", NO_REGEX);

    // Connect device and start TELEOP
    connect_virtual_device(DEVICE_NAME, UID);
    sleep(1);

    // Check that parameter is zero from the start
    param_val_t zero = {0};
    same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, zero);

    // Start TELEOP, which will attempt to write a nonzero value, but should fail
    send_run_mode(SHEPHERD, TELEOP);
    sleep(1);

    // Check that parameter remains zero
    same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, zero);
    sleep(1);
    same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, zero);

    // Connect Input, which should allow the write to happen
    float joysticks[4];
    send_user_input(0, joysticks, KEYBOARD);
    sleep(1);

    // Check that the parameter has changed
    param_val_t new_val = {.p_i = 999};
    same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, new_val);

    // Check that the run mode remained TELEOP throughout the whole test
    check_run_mode(TELEOP);
    return 0;
}
