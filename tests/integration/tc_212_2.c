#include "../test.h"

#define DEVICE_NAME "SimpleTestDevice"
#define UID 1
#define PARAM_NAME "MY_INT"

/**
 * Tests #212 (parameters that need to be emergency stopped)
 * Input: Connected > Disconnected > Reconnected
 * UserInput is initially connected before teleop setup begins, so Robot.set_value() will work in teleop_setup()
 * When UserInput is disconnected, writing nonzero values to emergency stopped parameters will be void and Robot must be stopped
 * When UserInput is reconnected, Robot.set_value() on these parameters should execute normally
 *  For this student code file, no Robot.set_value() is called, so the parameters will remain 0
 *  This test ensures that nonzero writes from before the STOP don't persist after the STOP, and only new writes are processed.
 * The Run Mode should also remain TELEOP throughout the whole test.
 */

int main() {
    start_test("Zero Params when Input Disconnected", "tc_212_b", NO_REGEX);

    // Connect device and start TELEOP
    connect_virtual_device(DEVICE_NAME, UID);
    sleep(1);

    // Check that parameter is zero from the start
    param_val_t zero = {0};
    same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, zero);

    // Connect Input, which should allow writes to happen when TELEOP starts
    float joysticks[4];
    send_user_input(0, joysticks, KEYBOARD);
    sleep(1);

    // Start TELEOP, which will write nonzero values successfully
    send_run_mode(SHEPHERD, TELEOP);
    sleep(1);

    // Check that the parameter has changed
    param_val_t new_val;
    new_val.p_i = 999;
    same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, new_val);

    // Disconnect the input source, which should stop the robot
    disconnect_user_input();
    sleep(1);

    // Check that parameter is set to zero and remains zero
    same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, zero);
    sleep(1);
    same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, zero);

    // Reconnect the input source, which will make parameters writable to nonzero values again
    send_user_input(0, joysticks, KEYBOARD);
    sleep(1);

    // For this student code file, there isn't another Robot.set_value executed, so it remains zero
    same_param_value(DEVICE_NAME, UID, PARAM_NAME, INT, zero);

    // Check that the run mode remained TELEOP throughout the whole test
    check_run_mode(TELEOP);
    return 0;
}
