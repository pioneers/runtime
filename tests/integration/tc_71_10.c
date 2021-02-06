/**
 * Performance test.
 * Calculates the latency between net handler receiving a button input and
 * lowcar processing a device write due to that button press.
 * Pressing "A" will write to "GET_TIME" param of TimeTestDevice
 * When TimeTestDevice's "GET_TIME" is set to 1, it populates its "TIMESTAMP"
 * param, which can be read from shared memory.
 * The latency should be no more than a couple milliseconds.
 */
#include "../test.h"

#define TIME_DEV_UID 123
#define UPPER_BOUND_LATENCY 5

int main() {
    // Setup
    start_test("Latency Test", "runtime_latency", "", NO_REGEX);

    // Connect TimeTestDevice
    connect_virtual_device("TimeTestDevice", TIME_DEV_UID);
    sleep(1);  // Let it connect

    // Connect gamepad
    uint32_t buttons = 0;
    float joystick_vals[4] = {0};
    send_user_input(buttons, joystick_vals, GAMEPAD);

    // Start teleop mode
    send_run_mode(SHEPHERD, TELEOP);

    // Start the timer and press A
    uint64_t start = millis();  // Start of timer
    buttons |= get_button_bit("button_a");
    send_user_input(buttons, joystick_vals, GAMEPAD);
    sleep(1);
    // Unpress "A"
    buttons = 0;
    send_user_input(buttons, joystick_vals, GAMEPAD);

    // Let processing happen
    sleep(1);

    // Check the latency between the button pressed and its change to TIMESTAMP
    check_latency(TIME_DEV_UID, UPPER_BOUND_LATENCY, start);

    // Stop all processes
    end_test();
    return 0;
}
