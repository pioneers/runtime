/**
 * Hotplugging/Performance
 * Since forming a connection with shm requires a virutal device to
 * disconnect then reconnect, not stay connected like in real situations,
 * we are looping a hot plug of bad devices to see if dev handler handles the devices
 * correctly and cleans up any threads for bad devices
 */
#include "../test.h"

char no_device[] = "no connected devices";
char unknown_device[] = "A non-PiE device was recently plugged in. Please unplug immediately";
char unknown_message[] = "Couldn't parse message from /tmp/ttyACM0";

int main(){
    // Setup
    start_test("Multiple Unstable Hotplug");
    start_shm();
    start_dev_handler();

    // Connect a device then disconnect it
    print_dev_ids();
    for(int i = 0; i < 3; i++){
        int j = 100 + i;
        connect_virtual_device("GeneralTestDevice", j);
    }
    sleep(1);
    print_dev_ids();
    for(int i = 0; i < 3; i++){
        int k = 150 + i;
        connect_virtual_device("UnstableTestDevice", k);
    }
    print_dev_ids();
    sleep(8);
    print_dev_ids();

    for(int i = 0; i < 2; i++){
        int z = 50 + i;
        switch(i){
            
            case 0:
            connect_virtual_device("ForeignTestDevice", z);

            case 1:
            connect_virtual_device("UnresponsiveTestDevice", z);

        }
    }
    sleep(8);
    print_dev_ids();


    disconnect_all_devices();
    sleep(1);
    print_dev_ids();
    stop_dev_handler();
    stop_shm();
    end_test();

    in_rest_of_output(no_device);  // No device connected
    char expected_output[64];
    in_rest_of_output("UnstableTestDevice (0x0000000000000096) timed out!");
    in_rest_of_output("UnstableTestDevice (0x0000000000000097) timed out!");
    in_rest_of_output("UnstableTestDevice (0x0000000000000098) timed out!");
    in_rest_of_output(unknown_device);
    in_rest_of_output(unknown_device);
    for (int i = 0; i < 3; i++) {
        sprintf(expected_output, "dev_ix = %d", i); // check that the only device connetcted are generalTestDevice
        in_rest_of_output(expected_output);
    }
    in_rest_of_output(no_device);
    return 0;
}