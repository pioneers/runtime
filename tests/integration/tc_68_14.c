/**
 * Hotplugging/Performance
 * Connecting different types of devices to "stress test" dev_handler
 * and see if it properly handles the multiple bad devices without affecting
 * the proper lowcar devices
 */
#include "../test.h"

char no_device[] = "no connected devices";
char unknown_device[] = "A non-PiE device was recently plugged in. Please unplug immediately";
char bad_message[] = "Couldn't parse message from /tmp/ttyACM0";

#define NUM_GENERAL 3
#define NUM_UNSTABLE 3

int main(){
    // Setup
    start_test("Hotplug Variety");
    start_shm();
    start_net_handler();
    start_dev_handler();
    uint8_t general_dev_type = device_name_to_type("GeneralTestDevice");
    uint8_t unstable_dev_type = device_name_to_type("UnstableTestDevice");
    uint64_t uid = 0;

    // Connect general devices
    print_dev_ids(); // No devices
    for (int i = 0; i < NUM_GENERAL; i++) {
        connect_virtual_device("GeneralTestDevice", uid);
        uid++;
    }
    sleep(1);
    print_dev_ids(); // Only GeneralTestDevices

    // connect unstable devices then sleep for 8 seconds
    for (int i = 0; i < NUM_UNSTABLE; i++) {
        connect_virtual_device("UnstableTestDevice", uid);
        uid++;
    }
    sleep(1);           // Make sure all devices are registered
    print_dev_ids();    // NUM_GENERAL + NUM_UNSTABLE devices
    sleep(5);           // All UnstableTestDevices should time out
    print_dev_ids();    // Only GeneralTestDevice
    connect_virtual_device("ForeignTestDevice", uid);
    uid++;
    connect_virtual_device("UnresponsiveTestDevice", uid);
    uid++;

    sleep(4);           // ForeignTestDevice and UnresponsiveTestDevice should be denied
    print_dev_ids();    // Only GeneralTestDevice

    // Clean up
    disconnect_all_devices();
    sleep(2);
    print_dev_ids();    // All devices should be properly disconnected
    sleep(1);
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    // Check outputs
    in_rest_of_output(no_device);  // No device connected
    char expected_output[64];
    // Only NUM_GENERAL GeneralTestDevices
    for (int i = 0; i < NUM_GENERAL; i++) {
        sprintf(expected_output, "dev_ix = %d: type = %d, year = %d, uid = %llu\n", i, general_dev_type, general_dev_type, i);
        in_rest_of_output(expected_output);
    }
    // After connecting NUM_UNSTABLE UnstableTestDevices
    for (int i = 0; i < NUM_GENERAL; i++) {
        sprintf(expected_output, "dev_ix = %d: type = %d, year = %d, uid = %llu\n", i, general_dev_type, general_dev_type, i);
        in_rest_of_output(expected_output);
    }
    for (int i = 0; i < NUM_UNSTABLE; i++) {
        sprintf(expected_output, "dev_ix = %d: type = %d, year = %d, uid = %llu\n", i + NUM_GENERAL, unstable_dev_type, unstable_dev_type, i + NUM_GENERAL);
        in_rest_of_output(expected_output);
    }
    // All UnstableTestDevices time out
    for (int i = 0; i < NUM_UNSTABLE; i++) {
        sprintf(expected_output, "UnstableTestDevice (0x%016llX) timed out!", i + NUM_GENERAL);
        in_rest_of_output(expected_output);
    }
    // Only GeneralTestDevices remain
    for (int i = 0; i < NUM_GENERAL; i++) {
        sprintf(expected_output, "dev_ix = %d: type = %d, year = %d, uid = %llu\n", i, general_dev_type, general_dev_type, i);
        in_rest_of_output(expected_output);
    }
    // ForeignTestDevice and UnresponsiveTestDevice should be denied
    in_rest_of_output(unknown_device);
    in_rest_of_output(unknown_device);
    // Only GeneralTestDevice remain
    for (int i = 0; i < NUM_GENERAL; i++) {
        sprintf(expected_output, "dev_ix = %d: type = %d, year = %d, uid = %llu\n", i, general_dev_type, general_dev_type, i);
        in_rest_of_output(expected_output);
    }
    // Disconnecting all device should show no devices
    in_rest_of_output(no_device);
    return 0;
}
