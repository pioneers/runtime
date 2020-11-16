/**
 * Hotplugging
 * Verifies that an UnresponsiveTestDevice does not get connected to shared memory
 * An UnresponsiveTestDevice does not send data.
 */
#include "../test.h"

#define UID 0x123

#define ORDERED_STRINGS 0
#define UNORDERED_STRINGS 0

int main() {
    // Setup
    start_test("Hotplug UnresponsiveTestDevice", "", "", ORDERED_STRINGS, UNORDERED_STRINGS, NO_REGEX);

    // Connect an UnresponsiveTestDevice
    check_device_not_connected(UID);
    connect_virtual_device("UnresponsiveTestDevice", UID);
    sleep(2);
    check_device_not_connected(UID);

    // Clean up
    end_test();
    return 0;
}
