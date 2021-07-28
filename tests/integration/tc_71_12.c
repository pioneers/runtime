/**
 * Test to verify that writing to read-only params do not change their value
 */
#include "../test.h"

#define UID 0x123

int main() {
    // Setup
    start_test("Invalid Write", "", NO_REGEX);

    // Connect a device
    char* dev_name = "SimpleTestDevice";
    connect_virtual_device(dev_name, UID);
    sleep(1);

    // Get identifiers
    uint8_t simple_type = device_name_to_type(dev_name);
    const int8_t doubling_idx = get_param_idx(simple_type, "DOUBLING");

    // Get current parameters then wait
    device_t* dev = get_device(simple_type);
    param_val_t vals_before[dev->num_params];
    device_read_uid(UID, EXECUTOR, DATA, (1 << doubling_idx), vals_before);

    // Write -1 to DOUBLING then wait (DOUBLING is read-only and should not change)
    param_val_t vals_to_write[dev->num_params];
    vals_to_write[doubling_idx].p_f = -1;
    device_write_uid(UID, EXECUTOR, COMMAND, (1 << doubling_idx), vals_to_write);
    sleep(1);  // Device values will change in this time

    // Verify DOUBLING changed as expected regardless of the write (write should not happen)
    vals_before[doubling_idx].p_f *= 2;
    same_param_value(dev_name, UID, "DOUBLING", FLOAT, vals_before[doubling_idx]);

    return 0;
}
