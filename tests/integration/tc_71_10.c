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

#define ORDERED_STRINGS 0
#define UNORDERED_STRINGS 0

int main() {
    // Setup
    start_test("Latency Test", "runtime_latency", "", ORDERED_STRINGS, UNORDERED_STRINGS);

    // Connect TimeTestDevice
    connect_virtual_device("TimeTestDevice", TIME_DEV_UID);
    sleep(1);  // Let it connect

    // Connect gamepad
    uint32_t buttons = 0;
    float joystick_vals[4] = {0};
    send_gamepad_state(buttons, joystick_vals);

    // Start teleop mode
    send_run_mode(SHEPHERD, TELEOP);

    // Start the timer and press A
    int32_t start = millis() % 1000000000;  // 9 digits, just like TimeTestDevice
    buttons |= (1 << BUTTON_A);
    send_gamepad_state(buttons, joystick_vals);

    // Unpress "A"
    buttons = 0;
    send_gamepad_state(buttons, joystick_vals);

    // Let processing happen
    printf("Pressed 'A' at time %d\n", start);
    sleep(1);

    // Read the timestamp (param 1) of when BUTTON_A was received on the device
    param_val_t params[2];
    device_read_uid(TIME_DEV_UID, EXECUTOR, DATA, 0b11, params);
    int32_t end = params[1].p_i;
    printf("Device received 'A' at time %d\n", end);

    printf("Latency: %d - %d == %d milliseconds\n", end, start, end - start);

    // Stop all processes
    end_test();
    return 0;
}
