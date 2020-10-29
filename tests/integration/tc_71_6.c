/**
 * Hotplugging/Performance test
 * Connects MAX_DEVICES devices then attempts to plug one more in
 * Verifies that devices registered in shared memory do not change after
 * connecting the extra device
 */
#include "../test.h"

#define NUM_TO_CONNECT MAX_DEVICES

void check_max_devices_connected() {
    for (int i = 0; i < NUM_TO_CONNECT; i++) {
        check_device_connected(i);
    }
}

int main() {
    // Setup
    start_test("Hotplug MAX_DEVICES + 1", "", "");

    // Connect MAX_DEVICES
    for (int i = 0; i < NUM_TO_CONNECT; i++) {
        connect_virtual_device("SimpleTestDevice", i);
    }
    sleep(2);         // Make sure all devices connect

    // Verify that uids 0 through 31 are connected
    check_max_devices_connected();

    // Let devices sit to make sure runtime can handle it
    printf("Letting devices sit\n");
    sleep(5);

    // Attempt to connect an extra device and verify it doesn't connect
    connect_virtual_device("SimpleTestDevice", 404);
    printf("Attempted to connect an extra device\n");
    sleep(2);         // Let device try to connect
    check_device_not_connected(404);

    // Verify all the first MAX_DEVICES devices are not disturbed
    check_max_devices_connected();

    // Stop all processes
    end_test();
    return 0;
}
