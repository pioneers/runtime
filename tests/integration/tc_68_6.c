/**
 * Hotplugging/Performance test
 * Connects MAX_DEVICES devices then attempts to plug one more in
 * Verifies that devices registered in shared memory do not change after
 * connecting the extra device
 */
#include "../test.h"

char no_device[] = "no connected devices";

void stop_test() {
    printf("Cleaning up\n");
    disconnect_all_devices();
    sleep(2);
    stop_dev_handler();
    stop_shm();
    end_test();
}

#define NUM_TO_CONNECT MAX_DEVICES

int main() {
    // Setup
    signal(SIGINT, stop_test);
    start_test("Hotplug MAX_DEVICES + 1");
    start_shm();
    start_dev_handler();

    // Connect MAX_DEVICES
    sleep(1);   // Make sure dev handler starts up
    for (int i = 0; i < NUM_TO_CONNECT; i++) {
        connect_device("SimpleTestDevice", i);
        if (i % 8 == 0) {
            sleep(1);
        }
    }
    sleep(10);
    print_dev_ids();
    printf("CONNECTED %d DEVICES\n", NUM_TO_CONNECT);
    connect_device("SimpleTestDevice", NUM_TO_CONNECT);
    sleep(2);
    printf("CONNECTED %d + 1 DEVICES\n", NUM_TO_CONNECT);
    print_dev_ids();

    // Stop all processes
    stop_test();

    // Check outputs
    char expected_output[64];
    for (int i = 0; i < NUM_TO_CONNECT; i++) {
        sprintf(expected_output, "dev_ix = %d", i); // MAX_DEVICES connected
        in_rest_of_output(expected_output);
    }
    return 0;
}
