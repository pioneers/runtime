/**
 * Hotplugging/Performance
 * Connecting different types of devices to "stress test" dev_handler
 * and see if it properly handles the multiple bad devices without affecting
 * the proper lowcar devices
 */
#include "../test.h"

#define NUM_GENERAL 15
#define NUM_UNSTABLE 3
#define NUM_BAD_DEVS 10

// Verifies that the initial NUM_GENERAL GeneralTestDevices are connected
void check_general_test_devices_connected() {
    for (int i = 0; i < NUM_GENERAL; i++) {
        check_device_connected(i);
    }
}

int main() {
    // Setup
    if (NUM_GENERAL + NUM_UNSTABLE > MAX_DEVICES) {
        printf("Invalid Number Of Devices Connected");
        exit(1);
    }
    start_test("Hotplug Variety", "", "", NO_REGEX);

    uint64_t uid = 0;

    // Connect NUM_GENERAL devices
    for (int i = 0; i < NUM_GENERAL; i++) {
        connect_virtual_device("GeneralTestDevice", uid);
        uid++;
    }
    sleep(1);

    // Verify
    check_general_test_devices_connected();

    // Connect NUM_UNSTABLE unstable devices
    for (int i = 0; i < NUM_UNSTABLE; i++) {
        connect_virtual_device("UnstableTestDevice", uid);
        uid++;
    }
    sleep(1);

    // Verify NUM_UNSTABLE unstable devices are connected
    for (int i = NUM_GENERAL; i < NUM_GENERAL + NUM_UNSTABLE; i++) {
        check_device_connected(i);
    }

    // Unstable devices should time out, and only GeneralTestDevices remain
    sleep(5);         // Unstable Devices should timeout
    check_general_test_devices_connected();
    for (int i = NUM_GENERAL; i < NUM_GENERAL + NUM_UNSTABLE; i++) {
        check_device_not_connected(i);
    }

    // Connect devices that dev handler should not connect to shm
    printf("Connecting ForeignTestDevices and UnresponsiveTestDevices\n");
    for (int i = 0; i < NUM_BAD_DEVS; i++) {
        if (uid % 2) {
            connect_virtual_device("ForeignTestDevice", uid);
        } else {
            connect_virtual_device("UnresponsiveTestDevice", uid);
        }
        uid++;
    }
    sleep(2);

    // Still only GeneralTestDevices should be connected
    check_general_test_devices_connected();
    for (int i = NUM_GENERAL; i < uid; i++) {
        check_device_not_connected(i);
    }

    // Clean up
    end_test();
    return 0;
}
