/**
 * Hotplugging
 * Verify that dev handler can properly recognize multiple UnstableTestDevices
 * timing out and cleans them up
 */
#include "../test.h"

#define NUM_TO_CONNECT 3

int main() {
    // Setup
    start_test("Hotplug Multiple UnstableTestDevices", "", "", NO_REGEX);

    // Connect multiple UnstableTestDevices
    for (int i = 0; i < NUM_TO_CONNECT; i++) {
        connect_virtual_device("UnstableTestDevice", i);
    }
    sleep(1);

    // Check that they are all present
    for (int i = 0; i < NUM_TO_CONNECT; i++) {
        check_device_connected(i);
    }

    // All devices will time out
    sleep(5);

    // Check that they all disconnected
    for (int i = 0; i < NUM_TO_CONNECT; i++) {
        check_device_not_connected(i);
    }

    return 0;
}
