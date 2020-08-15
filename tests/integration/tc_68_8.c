/**
 * Hotplugging
 * Verifies that a ForeignTestDevice does not get connected to shared memory.
 * A ForeignTestDevice does not send an ACK, but sends only random bytes
 */
#include "../test.h"

char no_device[] = "no connected devices";
char unknown_device[] = "A non-PiE device was recently plugged in. Please unplug immediately";
char bad_message[] = "Couldn't parse message from /tmp/ttyACM0";

int main() {
    // Setup
    start_test("Hotplug ForeignTestDevice");
    start_shm();
    start_net_handler();
    start_dev_handler();
    sleep(1);

    // Connect a ForeignTestDevice
    print_dev_ids(); // No device
    connect_virtual_device("ForeignTestDevice", 0x123); // Bad message, then unknown device
    sleep(2);
    print_dev_ids(); // No device

    // Clean up
    disconnect_all_devices();
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    // Check outputs
    in_rest_of_output(no_device);
    in_rest_of_output(bad_message);
    in_rest_of_output(unknown_device);
    in_rest_of_output(no_device);
    return 0;
}
