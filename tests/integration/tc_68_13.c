/**
 * Hotplugging
 * Test to make sure that dev_handler can clean up
 * threads properly even when multiple unresponsive devices are connected
 * then disconnected
 */
#include "../test.h"

char no_device[] = "no connected devices";
char unknown_message[] = "Couldn't parse message from /tmp/ttyACM0";

int main(){
    // Setup
    start_test("Multiple unstable Hotplug");
    start_shm();
    start_dev_handler();

    // Connect a device then disconnect it
    print_dev_ids();
    connect_virtual_device("UnstableTestDevice", 0x123);
    connect_virtual_device("UnstableTestDevice", 0x124);
    connect_virtual_device("UnstableTestDevice", 0x125);
    connect_virtual_device("UnstableTestDevice", 0x126);
    connect_virtual_device("UnstableTestDevice", 0x127);
    sleep(1);
    print_dev_ids();
    sleep(8);
    print_dev_ids();


    disconnect_all_devices();
    stop_dev_handler();
    stop_shm();
    end_test();

    in_rest_of_output(no_device);  // No device read
    char expected_output[64];
    for (int i = 0; i < 5; i++) {
        sprintf(expected_output, "dev_ix = %d", i); // check each unstable device is connected
        in_rest_of_output(expected_output);
    }
    in_rest_of_output("UnstableTestDevice (0x0000000000000123) timed out!");
    in_rest_of_output("UnstableTestDevice (0x0000000000000124) timed out!");
    in_rest_of_output("UnstableTestDevice (0x0000000000000125) timed out!");
    in_rest_of_output("UnstableTestDevice (0x0000000000000126) timed out!");
    in_rest_of_output("UnstableTestDevice (0x0000000000000127) timed out!");
    in_rest_of_output(no_device);
    return 0;
}