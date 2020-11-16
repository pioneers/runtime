/**
 * Hotplugging
 * Verifies that an UnstableTestDevice is connected to shared memory,
 * but dev handler recognizes that the device times out, and disconnects it
 * from shared memory.
 */
#include "../test.h"

#define UID 0x123

#define ORDERED_STRINGS 0
#define UNORDERED_STRINGS 0

int main() {
    // Setup
    start_test("Hotplug UnstableTestDevice", "", "", ORDERED_STRINGS, UNORDERED_STRINGS, NO_REGEX);

    // Connect UnstableTestDevice
    check_device_not_connected(UID);
    connect_virtual_device("UnstableTestDevice", UID);
    sleep(1);
    check_device_connected(UID);
    sleep(5);         // UnstableTestDevice will time out
    check_device_not_connected(UID);

    // Clean up
    end_test();
    return 0;
}
