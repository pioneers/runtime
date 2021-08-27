/**
 * Hotplugging
 * Verifies that a ForeignTestDevice does not get connected to shared memory.
 * A ForeignTestDevice does not send an ACK, and sends only random bytes
 */
#include "../test.h"

#define UID 0x123

int main() {
    // Setup
    start_test("Hotplug ForeignTestDevice", "", "", NO_REGEX);

    // Connect a ForeignTestDevice
    check_device_not_connected(UID);
    connect_virtual_device("ForeignTestDevice", UID);
    sleep(2);
    check_device_not_connected(UID);

    return 0;
}
