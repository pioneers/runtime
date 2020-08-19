/**
 * Hotplugging
 * Verifies that an UnstableTestDevice is connected to shared memory,
 * but dev handler recognizes that the device times out, and disconnects it
 * from shared memory.
 */
#include "../test.h"

char no_device[] = "no connected devices";
char timed_out[] = "UnstableTestDevice (0x0000000000000123) timed out!";

int main() {
    // Setup
    start_test("Hotplug UnstableTestDevice");
    start_shm();
    start_net_handler();
    start_dev_handler();
    sleep(1);

    // Connect UnstableTestDevice
    print_dev_ids();    // No device
    connect_virtual_device("UnstableTestDevice", 0x123);
    sleep(1);
    print_dev_ids();    // One device
    sleep(5);           // UnstableTestDevice will time out
    print_dev_ids();    // No device

    // Clean up
    disconnect_all_devices();
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    // Check output
    in_rest_of_output(no_device);  // No device read
    char expected_output[64];
    sprintf(expected_output, "dev_ix = 0: type = %d", device_name_to_type("UnstableTestDevice"));
    in_rest_of_output(expected_output);
    in_rest_of_output(timed_out);
    in_rest_of_output(no_device);
    return 0;
}
