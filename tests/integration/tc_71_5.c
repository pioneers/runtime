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
    start_net_handler();
    start_dev_handler();
    sleep(1);

    // Connect a device then disconnect it
    print_dev_ids();  // No device
    int socket_num = connect_virtual_device("SimpleTestDevice", 0x123);
    sleep(1);
    print_dev_ids();                        // One device
    disconnect_virtual_device(socket_num);  // Device will be at the 0th socket
    sleep(1);
    print_dev_ids();  // No device
    sleep(1);

    // Stop all processes
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    // Check outputs
    in_rest_of_output(no_device, NO_REGEX);  // Before connect
    char expected_output[64];
    sprintf(expected_output, "dev_ix = 0: type = %d", device_name_to_type("SimpleTestDevice"));
    in_rest_of_output(expected_output, NO_REGEX);
    in_rest_of_output(no_device, NO_REGEX);  // After disconnect
    return 0;
}
