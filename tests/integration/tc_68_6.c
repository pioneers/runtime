/**
 * Hotplugging/Performance test
 * Connects MAX_DEVICES devices then attempts to plug one more in
 * Verifies that devices registered in shared memory do not change after
 * connecting the extra device
 */
#include "../test.h"

#define NUM_TO_CONNECT MAX_DEVICES

int main() {
    // Setup
    start_test("Hotplug MAX_DEVICES + 1");
    start_shm();
    start_net_handler();
    start_dev_handler();
    sleep(1);

    // Connect MAX_DEVICES
    for (int i = 0; i < NUM_TO_CONNECT; i++) {
        connect_virtual_device("SimpleTestDevice", i);
    }
    sleep(2);   // Make sure all devices connect
    print_dev_ids(); // Should show MAX_DEVICES
    printf("CONNECTED %d DEVICES\n", NUM_TO_CONNECT);
    printf("Letting devices sit\n");
    sleep(5);   // Let devices sit to make sure runtime can handle it
    connect_virtual_device("SimpleTestDevice", NUM_TO_CONNECT);
    sleep(2);   // Let device connect
    print_dev_ids(); // Should show MAX_DEVICES (same as last print_dev_ids())
    printf("CONNECTED %d + 1 DEVICES\n", NUM_TO_CONNECT);
    sleep(2);   // Let last device sit

    // Stop all processes
    disconnect_all_devices();
    sleep(1);
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    // Check outputs. Same output TWICE because the extra device should have no effect
    char expected_output[64];
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < NUM_TO_CONNECT; i++) {
            sprintf(expected_output, "dev_ix = %d", i); // MAX_DEVICES connected
            in_rest_of_output(expected_output);
        }
    }
    return 0;
}
