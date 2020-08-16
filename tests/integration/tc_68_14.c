/**
 * Hotplugging/Performance
 * Connecting different types of devices to "stress test" dev_handler
 * and see if it properly handles the multiple bad devices without affecting
 * the proper lowcar devices
 */
#include "../test.h"

char no_device[] = "no connected devices";
char unknown_device[] = "A non-PiE device was recently plugged in. Please unplug immediately";

#define NUM_GENERAL 15
#define NUM_UNSTABLE 3
#define NUM_BAD_DEVS 10

int main(){
    // Setup
    if(NUM_GENERAL + NUM_UNSTABLE > MAX_DEVICES){
        printf("Invalid Number Of Devices Connected");
        exit(1);
    }
    start_test("Hotplug Variety");
    start_shm();
    start_net_handler();
    start_dev_handler();
    uint8_t general_dev_type = device_name_to_type("GeneralTestDevice");
    uint8_t unstable_dev_type = device_name_to_type("UnstableTestDevice");
    uint64_t uid = 0;

    
    print_dev_ids(); // No devices
    // Connect NUM_GENERAL devices
    for (int i = 0; i < NUM_GENERAL; i++) {
        connect_virtual_device("GeneralTestDevice", uid);
        uid++;
    }
    sleep(1);
    print_dev_ids(); // Only General Devices Connected
    
    // Connect NUM_UNSTABLE unstable devices then sleep for 5 seconds
    for(int i = 0; i < NUM_UNSTABLE; i++){
        connect_virtual_device("UnstableTestDevice", uid);
        uid++;
    }
    sleep(2);
    print_dev_ids();
    sleep(5); // Unstable Devices should timeout
    
    // Disconnect the unstable devices
    for(int i = NUM_GENERAL; i < NUM_UNSTABLE; i++){
        disconnect_virtual_device(i);
    }
    sleep(1);
    print_dev_ids();

    // Connect "bad" devices from the ports of disconnected unstable devices
    for(int i = 0; i < NUM_BAD_DEVS; i++){
        if(uid % 2){
            connect_virtual_device("ForeignTestDevice", uid);
        }
        else{
            connect_virtual_device("UnresponsiveTestDevice", uid);
        }
        uid++;
        sleep(1); // Let Dev_Handler properly handlt the device before connecting another
    }
    sleep(2);
    print_dev_ids(); // Only general devices should be connected


    // Clean up
    disconnect_all_devices();
    sleep(8);
    print_dev_ids(); // All Devices should be disconnected
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    in_rest_of_output(no_device);  // No devices initially connected
    char expected_output[64];
    char unexpected_output[64];
    //Only NUM_GENERAL GeneralTestDevices
    // for (int i = 0; i < NUM_GENERAL - 1; i++) {
    //     sprintf(expected_output, "dev_ix = %d: type = %d, year = %d, uid = %llu\n", i, general_dev_type, general_dev_type, i);
    //     in_rest_of_output(expected_output);
    // }
    // // Check NUM_GENERAL devices are connected then NUM_UNSTABLE
    // for (int i = 0; i < NUM_GENERAL; i++) {
    //     sprintf(expected_output, "dev_ix = %d: type = %d, year = %d, uid = %llu\n", i, general_dev_type, general_dev_type, i);
    //     in_rest_of_output(expected_output);
    // }
    for (int i = NUM_GENERAL; i < NUM_UNSTABLE; i++) {
        sprintf(expected_output, "dev_ix = %d: type = %d, year = %d, uid = %llu\n", i, unstable_dev_type, unstable_dev_type, i);
        in_rest_of_output(expected_output);
    }
    // After all UnstableTestDevices timed out
    for (int i = NUM_GENERAL; i < NUM_UNSTABLE + NUM_GENERAL; i++) {
        sprintf(expected_output, "UnstableTestDevice (0x%016llX) timed out!", (i)); 
        in_rest_of_output(expected_output);
    }
    // Only GeneralTestDevices remain after disconnecting UnstableTestDevices
    for (int i = 0; i < NUM_GENERAL; i++) {
        sprintf(expected_output, "dev_ix = %d: type = %d, year = %d, uid = %llu\n", i, general_dev_type, general_dev_type, i);
        in_rest_of_output(expected_output);
    }
    // Make sure there are no other devices in SHM
    for (int i = NUM_GENERAL; i < MAX_DEVICES; i++) {
        sprintf(unexpected_output, "dev_ix = %d", i);
        not_in_rest_of_output(unexpected_output);
    }
    // Error message when non pie devices are connected
    for (int i = 0; i < NUM_BAD_DEVS; i++){
        in_rest_of_output(unknown_device);
    }
    // Only GeneralTestDevices are in SHM after bad devices are handled
    for (int i = 0; i < NUM_GENERAL; i++) {
        sprintf(expected_output, "dev_ix = %d: type = %d, year = %d, uid = %llu\n", i, general_dev_type, general_dev_type, i);
        in_rest_of_output(expected_output);
    }
    // Make sure there are no other devices in SHM
    for (int i = NUM_GENERAL; i < MAX_DEVICES; i++) {
        sprintf(unexpected_output, "dev_ix = %d", i);
        not_in_rest_of_output(unexpected_output);
    }
    in_rest_of_output(no_device); // All Devices Disconnected
    return 0;
}
