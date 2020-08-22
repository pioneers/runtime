/**
 * Hotplugging/Performance
 * Connecting different types of devices to "stress test" dev_handler
 * and see if it properly handles the multiple bad devices without affecting
 * the proper lowcar devices
 */
#include "../test.h"

char no_device[] = "no connected devices";
char unknown_device[] = "A non-PiE device was recently plugged in. Please unplug immediately";
char print_dev_ids_format[] = "dev_ix = %d: type = %d, year = %d, uid = %llu\n";

#define NUM_GENERAL 15
#define NUM_UNSTABLE 3
#define NUM_BAD_DEVS 10

int main() {
    // Setup
    if (NUM_GENERAL + NUM_UNSTABLE > MAX_DEVICES) {
        printf("Invalid Number Of Devices Connected");
        exit(1);
    }
    start_test("Hotplug Variety");
    start_shm();
    start_net_handler();
    start_dev_handler();
    sleep(1);

    uint8_t general_dev_type = device_name_to_type("GeneralTestDevice");
    uint8_t unstable_dev_type = device_name_to_type("UnstableTestDevice");
    uint64_t uid = 0;

    print_dev_ids();  // No initial devices
    // Connect NUM_GENERAL devices
    for (int i = 0; i < NUM_GENERAL; i++) {
        connect_virtual_device("GeneralTestDevice", uid);
        uid++;
    }
    sleep(1);
    print_dev_ids();  // Only GeneralTestDevices

    // Connect NUM_UNSTABLE unstable devices then let them time out
    for (int i = 0; i < NUM_UNSTABLE; i++) {
        connect_virtual_device("UnstableTestDevice", uid);
        uid++;
    }
    sleep(1);
    print_dev_ids();  // NUM_GENERAL + NUM_UNSTABLE devices
    sleep(5);         // Unstable Devices should timeout
    print_dev_ids();  // Only GeneralTestDevices

    // Disconnect the unstable devices
    for (int i = NUM_GENERAL; i < NUM_UNSTABLE; i++) {
        disconnect_virtual_device(i);
    }
    sleep(1);

    // Connect devices that dev handler should not connect to shm
    for (int i = 0; i < NUM_BAD_DEVS; i++) {
        if (uid % 2) {
            connect_virtual_device("ForeignTestDevice", uid);
        } else {
            connect_virtual_device("UnresponsiveTestDevice", uid);
        }
        uid++;
    }
    sleep(2);
    print_dev_ids();  // Only GeneralTestDevices

    // Clean up
    disconnect_all_devices();
    sleep(2);
    print_dev_ids();  // All Devices should be disconnected
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    in_rest_of_output(no_device);  // No devices initially connected
    char expected_output[64];
    char unexpected_output[64];
    //Only NUM_GENERAL GeneralTestDevices
    for (int i = 0; i < NUM_GENERAL - 1; i++) {
        sprintf(expected_output, print_dev_ids_format, i, general_dev_type, general_dev_type, (uint64_t) i);
        in_rest_of_output(expected_output);
    }
    // Check both NUM_GENERAL GeneralTestDevices and NUM_UNSTABLE UnstableTestDevices are connected
    for (int i = 0; i < NUM_GENERAL; i++) {
        sprintf(expected_output, print_dev_ids_format, i, general_dev_type, general_dev_type, (uint64_t) i);
        in_rest_of_output(expected_output);
    }
    for (int i = NUM_GENERAL; i < NUM_UNSTABLE; i++) {
        sprintf(expected_output, print_dev_ids_format, i, unstable_dev_type, unstable_dev_type, (uint64_t) i);
        in_rest_of_output(expected_output);
    }
    // All UnstableTestDevices should time out
    for (int i = NUM_GENERAL; i < NUM_UNSTABLE + NUM_GENERAL; i++) {
        sprintf(expected_output, "UnstableTestDevice (0x%016llX) timed out!", (uint64_t) i);
        in_rest_of_output(expected_output);
    }
    // GeneralTestDevices remain in shm after UnstableTestDevices time out
    for (int i = 0; i < NUM_GENERAL; i++) {
        sprintf(expected_output, print_dev_ids_format, i, general_dev_type, general_dev_type, (uint64_t) i);
        in_rest_of_output(expected_output);
    }
    // Make sure there are no other devices in SHM
    for (int i = NUM_GENERAL; i < MAX_DEVICES; i++) {
        sprintf(unexpected_output, "dev_ix = %d", i);
        not_in_rest_of_output(unexpected_output);
    }
    // Error message when ForeignTestDevices and UnresponsiveTestDevices are connected
    for (int i = 0; i < NUM_BAD_DEVS; i++) {
        in_rest_of_output(unknown_device);
    }
    // GeneralTestDevices remain in SHM after bad devices are handled
    for (int i = 0; i < NUM_GENERAL; i++) {
        sprintf(expected_output, print_dev_ids_format, i, general_dev_type, general_dev_type, (uint64_t) i);
        in_rest_of_output(expected_output);
    }
    // Make sure there are no other devices in SHM besides GeneralTestDevices
    for (int i = NUM_GENERAL; i < MAX_DEVICES; i++) {
        sprintf(unexpected_output, "dev_ix = %d", i);
        not_in_rest_of_output(unexpected_output);
    }
    in_rest_of_output(no_device);  // All Devices Disconnected properly
    return 0;
}
