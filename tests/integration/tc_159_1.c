/**
 * Test to verify that executor properly subscribes to all parameters
 * requested by student code calls to Robot.get_value()
 */
#include "../test.h"

#define SIMPLE_UID 2
#define GENERAL_UID 1

int main() {
    // Setup
    start_test("Executor Subscriptions", "subscriptions", NO_REGEX);

    // Connect SimpleTestDevice and GeneralTestDevice
    connect_virtual_device("SimpleTestDevice", SIMPLE_UID);
    connect_virtual_device("GeneralTestDevice", GENERAL_UID);
    sleep(1);

    // Set run mode AUTO
    send_run_mode(SHEPHERD, AUTO);
    sleep(1);

    // Verify subscriptions
    // Expected subscriptions in subscriptions.py:
    //   62_2: ['FLIP_FLOP', 'DOUBLING', 'INCREASING]
    //   63_1: ['EXP_ONE_PT_ONE', 'INCREASING_ODD', 'DECREASING_ODD']
    uint8_t simple_type = device_name_to_type("SimpleTestDevice");
    uint32_t simple_subs = (1 << get_param_idx(simple_type, "FLIP_FLOP")) | (1 << get_param_idx(simple_type, "DOUBLING")) | (1 << get_param_idx(simple_type, "INCREASING"));
    check_sub_requests(SIMPLE_UID, simple_subs, EXECUTOR);

    uint8_t general_type = device_name_to_type("GeneralTestDevice");
    uint32_t general_subs = (1 << get_param_idx(general_type, "EXP_ONE_PT_ONE")) | (1 << get_param_idx(general_type, "INCREASING_ODD")) | (1 << get_param_idx(general_type, "DECREASING_ODD"));
    check_sub_requests(GENERAL_UID, general_subs, EXECUTOR);

    return 0;
}
