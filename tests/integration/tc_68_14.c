/**
 * Hotplugging/Performance
 * Connecting different types of devices to "stress test" dev_handler
 * and see if it properly handles the multiple bad devies
 * (Ideally we would connect more devices but the "phenomenom" has said no)
 */
#include "../test.h"

char no_device[] = "no connected devices";
char unknown_device[] = "A non-PiE device was recently plugged in. Please unplug immediately";
char unknown_message[] = "Couldn't parse message from /tmp/ttyACM0";

int main(){
    // Setup
    start_test("Multiple Unstable Hotplug");
    start_shm();
    start_net_handler();
    start_dev_handler();
    uint64_t uid = 0;

    // Connect general devices
    print_dev_ids();
    for(int i = 0; i < 3; i++){
        uid++;
        connect_virtual_device("GeneralTestDevice", uid);
    }
    sleep(1);
    print_dev_ids();
    
    //connect unstable devices then sleep for 8 seconds
    for(int i = 0; i < 3; i++){
        uid++;
        connect_virtual_device("UnstableTestDevice", uid);
    }
    print_dev_ids();
    sleep(8);
    print_dev_ids();
    connect_virtual_device("ForeignTestDevice", uid);
    uid++;
    connect_virtual_device("UnresponsiveTestDevice", uid);

    sleep(8);
    print_dev_ids();


    disconnect_all_devices();
    sleep(1);
    print_dev_ids();
    sleep(1);
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    in_rest_of_output(no_device);  // No device connected
    char expected_output[64];
    in_rest_of_output("UnstableTestDevice (0x0000000000000004) timed out!");
    in_rest_of_output("UnstableTestDevice (0x0000000000000005) timed out!");
    in_rest_of_output("UnstableTestDevice (0x0000000000000006) timed out!");
    in_rest_of_output(unknown_device);
    in_rest_of_output(unknown_device);
    for (int i = 0; i < 3; i++) {
        sprintf(expected_output, "dev_ix = %d", i); // check that the only device connetcted are generalTestDevice
        in_rest_of_output(expected_output);
    }
    in_rest_of_output(no_device);
    return 0;
}