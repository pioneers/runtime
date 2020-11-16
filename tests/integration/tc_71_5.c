/**
 * Simple Hotplugging test to verify a single device connect/disconnect
 * registers in shared memory
 */
#include "../test.h"

#define ORDERED_STRINGS 0
#define UNORDERED_STRINGS 0

#define UID 0x123

int main() {
    start_test("Simple Hotplug", "", "", ORDERED_STRINGS, UNORDERED_STRINGS, NO_REGEX);

    // Connect a device then disconnect it
    check_device_not_connected(UID);
    int socket_num = connect_virtual_device("SimpleTestDevice", UID);
    sleep(1);
    check_device_connected(UID);
    disconnect_virtual_device(socket_num);
    sleep(1);
    check_device_not_connected(UID);

    end_test();
    return 0;
}
