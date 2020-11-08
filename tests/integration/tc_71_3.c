#include "../test.h"

/**
 * This test checks to make sure that device subscriptions made to nonexistent devices
 * from Dawn are rejected by shared memory and an error message is sent back to Dawn.
 */

#define ORDERED_STRINGS 4
#define UNORDERED_STRINGS 0

char check_output_1[] = "no device at dev_uid = 50, sub request failed\n";

char check_output_2[] = "recv_new_msg: Invalid device subscription, device uid 50 is invalid\n";

char check_output_3[] = "no device at dev_uid = 100, sub request failed\n";

char check_output_4[] = "recv_new_msg: Invalid device subscription, device uid 100 is invalid\n";

int main() {
    // setup
    start_test("nonexistent device subscription", "", "", ORDERED_STRINGS, UNORDERED_STRINGS);

    // poke
    dev_subs_t data1 = {.uid = 50, .name = "ServoControl", .params = 0b11};
    add_ordered_string_output(check_output_1);
    dev_subs_t data2 = {.uid = 100, .name = "LimitSwitch", .params = 0b101};
    add_ordered_string_output(check_output_2);
    dev_subs_t data_total[2] = {data1, data2};
    add_ordered_string_output(check_output_3);
    send_device_subs(data_total, 2);
    add_ordered_string_output(check_output_4);
    // stop
    end_test();

    // check output
    return 0;
}
