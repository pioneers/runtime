/**
 * Hotplugging
 * Test to make sure that an 'Unstable' device gets skipped
 * by dev handler and isnt connected to shm
 */
#include "../test.h"

char no_device[] = "no connected devices";

int main(){
    // Setup
    start_test("Unstable Hotplug");
    start_shm();
    start_dev_handler();

    // Connect a device then disconnect it
    print_dev_ids();
    connect_virtual_device("UnstableTestDevice", 0x123);
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
    sprintf(expected_output, "dev_ix = 0: type = %d", device_name_to_type("UnstableTestDevice"));
    in_rest_of_output(expected_output);
    in_rest_of_output(no_device);
    return 0;
}