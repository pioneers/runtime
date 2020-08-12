/**
 * Hotplugging
 * Test to make sure that an 'Unresponsive' is not connected
 * to shared memory
 */
#include "../test.h"

char no_device[] = "no connected devices";
char unknown_device[] = "A non-PiE device was recently plugged in. Please unplug immediately";

int main(){
    // Setup
    start_test("Unresponsive Hotplug");
    start_shm();
    start_dev_handler();

    // Connect a device then check if it is not recognized
    print_dev_ids();
    connect_virtual_device("UnresponsiveTestDevice", 0x123);
    sleep(2);
    print_dev_ids();

    disconnect_all_devices();
    stop_dev_handler();
    stop_shm();
    end_test();
    
    in_rest_of_output(no_device);
    in_rest_of_output(unknown_device);
    char unexpected_output[64];
    sprintf(unexpected_output, "dev_ix = 0: type = %d", device_name_to_type("UnresponsiveTestDevice"));
    not_in_output(unexpected_output);  // No device connected to shm
    return 0;
}