/**
 * Simple read test to verify that shared memory is updated with new device data
 * A SimpleTestDevice's read-only params change every second
 */
#include "../test.h"

#define UID 0x123

int main() {
    // Setup
    start_test("Simple Device Read", "", "");

    // Connect a device
    char *dev_name = "SimpleTestDevice";
    connect_virtual_device(dev_name, UID);
    sleep(1);

    // Get current parameters then wait
    uint8_t dev_type = device_name_to_type(dev_name);
    device_t* dev = get_device(dev_type);
    param_val_t vals_before[dev->num_params];
    device_read_uid(UID, EXECUTOR, DATA, 0b1111, vals_before);
    sleep(1);  // Device values will change in this time

    // Get parameters again
    // param_val_t vals_after[dev->num_params];
    // device_read_uid(UID, EXECUTOR, DATA, 0b1111, vals_after);

    // Verify parameters changed as expected
    printf("Going to check increasing");
    vals_before[0].p_i++;  // INCREASING: Increased by 1
    same_param_value(dev_name, UID, "INCREASING", INT, vals_before[0]);

    vals_before[1].p_f *= 2;  // DOUBLING: Doubled value
    same_param_value(dev_name, UID, "DOUBLING", FLOAT, vals_before[1]);

    vals_before[2].p_b = 1 - vals_before[2].p_b;  // FLIP_FLOP: Opposite truth value
    same_param_value(dev_name, UID, "FLIP_FLOP", BOOL, vals_before[2]);

    // Stop all processes
    end_test();

    

    return 0;
}
