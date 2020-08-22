/**
 * Simple read test to verify that shared memory is updated with new device data
 * A SimpleTestDevice's read-only params change every second
 */
#include "../test.h"

#define UID 0x123

int main() {
    // Setup
    start_test("Simple Device Read");
    start_shm();
    start_net_handler();
    start_dev_handler();

    // Connect a device
    connect_virtual_device("SimpleTestDevice", UID);
    sleep(1);

    // Get current parameters then wait
    device_t* dev = get_device(device_name_to_type("SimpleTestDevice"));
    param_val_t vals_before[dev->num_params];
    device_read_uid(UID, EXECUTOR, DATA, 0b1111, vals_before);
    sleep(1);  // Device values will change in this time

    // Get parameters again
    param_val_t vals_after[dev->num_params];
    device_read_uid(UID, EXECUTOR, DATA, 0b1111, vals_after);

    // Stop all processes
    disconnect_all_devices();
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    // Verify parameters changed as expected
    vals_before[0].p_i++;  // INCREASING: Increased by 1
    same_param_value("INCREASING", INT, vals_before[0], vals_after[0]);

    vals_before[1].p_f *= 2;  // DOUBLING: Doubled value
    same_param_value("DOUBLING", FLOAT, vals_before[1], vals_after[1]);

    vals_before[2].p_b = 1 - vals_before[2].p_b;  // FLIP_FLOP: Opposite truth value
    same_param_value("FLIP_FLOP", BOOL, vals_before[2], vals_after[2]);

    return 0;
}
