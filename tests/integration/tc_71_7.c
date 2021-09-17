/**
 * Hotplugging
 * Verifies that an UnresponsiveTestDevice does not get connected to shared memory
 * An UnresponsiveTestDevice does not send data.
 */
#include "../test.h"

#define UID 0x123

int main() {
    // Setup
    start_test("Hotplug UnresponsiveTestDevice", "", NO_REGEX);

    // Connect an UnresponsiveTestDevice
    check_device_not_connected(UID);
    connect_virtual_device("UnresponsiveTestDevice", UID);
    sleep(2);
    check_device_not_connected(UID);

    return 0;
}
