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
    connect_device("UnstableTestDevice", 0x123);
    sleep(15);
    print_dev_ids();

    stop_dev_handler();
    stop_shm();
    end_test();

    in_rest_of_output(no_device);  // No device read
    return 0;
}