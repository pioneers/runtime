#include "../test.h"

/**
 * This test ensures that received device data on UDP from the
 * perspective of Dawn is as expected when devices are connected
 * and disocnnected by the student.
 */

#define UID1 0x1234
#define UID2 0x2341
#define UID3 0x3412
#define UID4 0x4123

// We don't check the contents of the arriving data, we only check that
// the devices that we expect to be present are coming in.

// check that first two devices connected
char check_1_output[] = "Connected SimpleTestDevice (0x0000000000001234) from year 62!\n";
char check_2_output[] = "Connected SimpleTestDevice (0x0000000000002341) from year 62!\n";

// check in device data that those are the two devices we see
char check_3_output[] = "Device No. 0:\ttype = SimpleTestDevice, uid = 4660, itype = 62\n";
char check_4_output[] = "Device No. 1:\ttype = SimpleTestDevice, uid = 9025, itype = 62\n";

// check that first device disconnected
char check_5_output[] = "SimpleTestDevice (0x0000000000001234) disconnected!\n";

// check in device data that second device only is reported
char check_6_output[] = "Device No. 0:\ttype = SimpleTestDevice, uid = 9025, itype = 62\n";

// check that next two devices connected
char check_7_output[] = "Connected SimpleTestDevice (0x0000000000003412) from year 62!\n";
char check_8_output[] = "Connected SimpleTestDevice (0x0000000000004123) from year 62!\n";

// check that we see three devices in output
char check_9_output[] = "Device No. 0:\ttype = SimpleTestDevice, uid = 13330, itype = 62\n";
char check_10_output[] = "Device No. 1:\ttype = SimpleTestDevice, uid = 9025, itype = 62\n";
char check_11_output[] = "Device No. 2:\ttype = SimpleTestDevice, uid = 16675, itype = 62\n";

// check that all three devices disconnected
char check_12_output[] = "SimpleTestDevice (0x0000000000003412) disconnected!\n";
char check_13_output[] = "SimpleTestDevice (0x0000000000002341) disconnected!\n";
char check_14_output[] = "SimpleTestDevice (0x0000000000004123) disconnected!\n";

// check that last device data has only the custom data device
char check_15_not_output[] = "SimpleTestDevice";
char check_16_output[] =
    "Device No. 0:\ttype = CustomData, uid = 2020, itype = 32\n"
    "\tParams:\n"
    "\t\tparam \"time_ms\" has type INT with value";

int main() {
    int port1, port2, port3;

    // setup
    start_test("receive device data, general");
    start_shm();
    start_net_handler();
    start_dev_handler();

    // poke the system
    // send gamepad state so net_handler starts sending device data packets
    uint32_t buttons = 0;
    float joystick_vals[] = {0.0, 0.0, 0.0, 0.0};
    send_gamepad_state(buttons, joystick_vals);
    sleep(1);
    print_next_dev_data();

    // connect two devices
    port1 = connect_virtual_device("SimpleTestDevice", UID1);
    port2 = connect_virtual_device("SimpleTestDevice", UID2);

    // print device data
    sleep(1);
    print_next_dev_data();

    // disconnect first device
    disconnect_virtual_device(port1);

    // print device data
    sleep(1);
    print_next_dev_data();

    // connect two more devices
    port1 = connect_virtual_device("SimpleTestDevice", UID3);
    port3 = connect_virtual_device("SimpleTestDevice", UID4);

    // print device data
    sleep(1);
    print_next_dev_data();
    sleep(1);

    // disconnect all devices
    disconnect_virtual_device(port1);
    usleep(500000);
    disconnect_virtual_device(port2);
    usleep(500000);
    disconnect_virtual_device(port3);

    // print device data
    sleep(1);
    print_next_dev_data();

    // stop the system
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    // check output
    in_rest_of_output(check_1_output, NO_REGEX);
    in_rest_of_output(check_2_output, NO_REGEX);
    in_rest_of_output(check_3_output, NO_REGEX);
    in_rest_of_output(check_4_output, NO_REGEX);
    in_rest_of_output(check_5_output, NO_REGEX);
    in_rest_of_output(check_6_output, NO_REGEX);
    in_rest_of_output(check_7_output, NO_REGEX);
    in_rest_of_output(check_8_output, NO_REGEX);
    in_rest_of_output(check_9_output, NO_REGEX);
    in_rest_of_output(check_10_output, NO_REGEX);
    in_rest_of_output(check_11_output, NO_REGEX);
    in_rest_of_output(check_12_output, NO_REGEX);
    in_rest_of_output(check_13_output, NO_REGEX);
    in_rest_of_output(check_14_output, NO_REGEX);
    not_in_rest_of_output(check_15_not_output, NO_REGEX);
    in_rest_of_output(check_16_output, NO_REGEX);
}
