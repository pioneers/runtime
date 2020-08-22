/**
 * Hotplugging
 * Verifies that an UnresponsiveTestDevice does not get connected to shared memory
 * An UnresponsiveTestDevice does not send data.
 */
#include "../test.h"

char no_device[] = "no connected devices";
char unknown_device[] = "A non-PiE device was recently plugged in. Please unplug immediately";

int main() {
    // Setup
    start_test("Hotplug UnresponsiveTestDevice");
    start_shm();
    start_net_handler();
    start_dev_handler();
    sleep(1);

    // Connect an UnresponsiveTestDevice
    print_dev_ids();                                          // No device
    connect_virtual_device("UnresponsiveTestDevice", 0x123);  // Unknown device
    sleep(2);
    print_dev_ids();  // No device

    // Clean up
    disconnect_all_devices();
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    in_rest_of_output(no_device);
    in_rest_of_output(unknown_device);
    in_rest_of_output(no_device);
    return 0;
}
