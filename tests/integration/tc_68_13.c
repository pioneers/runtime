/**
 * Hotplugging
 * Verify that dev handler can properly recognize multiple UnstableTestDevices
 * timing out and cleans them up
 */
#include "../test.h"

#define NUM_TO_CONNECT 4

char no_device[] = "no connected devices";
char unknown_message[] = "Couldn't parse message from /tmp/ttyACM0";

int main() {
    // Setup
    start_test("Hotplug Multiple UnstableTestDevices");
    start_shm();
    start_net_handler();
    start_dev_handler();

    // Connect multiple UnstableTestDevices
    for (int i = 0; i < NUM_TO_CONNECT; i++) {
        connect_virtual_device("UnstableTestDevice", i);
    }
    sleep(1);
    print_dev_ids(); // All devices should be present
    sleep(5);        // All devices will time out
    print_dev_ids(); // No devices

    // Clean up
    disconnect_all_devices();
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    // Check output
    char expected_output[64];
    for (int i = 0; i < NUM_TO_CONNECT; i++) {
        sprintf(expected_output, "dev_ix = %d: type = %d", i, device_name_to_type("UnstableTestDevice")); // check each unstable device is connected
        in_rest_of_output(expected_output);
    }
    for (int i = 0; i < NUM_TO_CONNECT; i++) {
        sprintf("UnstableTestDevice (0x%016llu) timed out!", i);
        in_rest_of_output(expected_output);
    }
    in_rest_of_output(no_device);
    return 0;
}
