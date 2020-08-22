/**
 * Test to verify that writing to read-only params do not change their value
 */
#include "../test.h"

#define UID 0x123

int main() {
    // Setup
    start_test("Invalid Write");
    start_shm();
    start_net_handler();
    start_dev_handler();

    // Connect a device
    connect_virtual_device("SimpleTestDevice", UID);
    sleep(1);

    // Get identifiers
    uint8_t simple_type = device_name_to_type("SimpleTestDevice");
    const int8_t doubling_idx = get_param_idx(simple_type, "DOUBLING");

    // Get current parameters then wait
    device_t* dev = get_device(simple_type);
    param_val_t vals_before[dev->num_params];
    device_read_uid(UID, EXECUTOR, DATA, (1 << doubling_idx), vals_before);

    // Write -1 to DOUBLING then wait
    param_val_t vals_to_write[dev->num_params];
    vals_to_write[doubling_idx].p_f = -1;
    device_write_uid(UID, EXECUTOR, COMMAND, (1 << doubling_idx), vals_to_write);
    sleep(1);  // Device values will change in this time

    // Get parameters again
    param_val_t vals_after[dev->num_params];
    device_read_uid(UID, EXECUTOR, DATA, (1 << doubling_idx), vals_after);

    // Stop all processes
    disconnect_all_devices();
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    // Verify DOUBLING changed as expected regardless of the write
    vals_before[doubling_idx].p_f *= 2;
    same_param_value("DOUBLING", FLOAT, vals_before[doubling_idx], vals_after[doubling_idx]);

    return 0;
}
