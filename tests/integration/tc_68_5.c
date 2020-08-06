/**
 * Simple Hotplugging test to verify a single device connect/disconnect
 * registers in shared memory
 */
#include "../test.h"

char no_device[] = "no connected devices";

int main() {
    // Setup
    start_test("Simple Hotplug");
    start_shm();
    start_dev_handler();

    // Connect a device then disconnect it
    print_dev_ids();
    int socket_num = connect_virtual_device("SimpleTestDevice", 0x123);
    sleep(1);
    print_dev_ids();
    disconnect_virtual_device(socket_num);   // Device will be at the 0th socket
    sleep(1);
    print_dev_ids();

    // Stop all processes
    stop_dev_handler();
    stop_shm();
    end_test();

    // Check outputs
    in_rest_of_output(no_device);  // Before connect
    char expected_output[64];
    sprintf(expected_output, "dev_ix = 0: type = %d", device_name_to_type("SimpleTestDevice"));
    in_rest_of_output(expected_output);
    in_rest_of_output(no_device);  // After disconnect
    return 0;
}
