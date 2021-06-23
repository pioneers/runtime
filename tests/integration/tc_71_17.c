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

int main() {
    int port1, port2, port3;

    // setup
    start_test("receive device data, general", "", "", NO_REGEX);

    // poke the system
    // send gamepad state so net_handler starts sending device data packets
    uint32_t buttons = 0;
    float joystick_vals[] = {0.0, 0.0, 0.0, 0.0};
    send_user_input(buttons, joystick_vals, GAMEPAD);
    sleep(1);

    // connect two devices
    port1 = connect_virtual_device("SimpleTestDevice", UID1);
    sleep(1);
    check_device_connected(UID1);

    port2 = connect_virtual_device("SimpleTestDevice", UID2);
    sleep(1);
    check_device_connected(UID2);

    // print device data
    sleep(1);
    DevData* dev_data = get_next_dev_data();
    // check in device data that those are the two devices we see
    check_device_sent(dev_data, 0, 62, 4660);
    check_device_sent(dev_data, 1, 62, 9025);
    dev_data__free_unpacked(dev_data, NULL);

    // disconnect first device (Port1=Empty, Port2=UID2)
    disconnect_virtual_device(port1);
    sleep(1);
    check_device_not_connected(UID1);

    // print device data
    sleep(1);
    dev_data = get_next_dev_data();
    // check in device data that second device only is reported
    check_device_sent(dev_data, 0, 62, 9025);
    dev_data__free_unpacked(dev_data, NULL);

    // connect two more devices (Port1=UID3, Port2=UID2, Port3=UID4)
    port1 = connect_virtual_device("SimpleTestDevice", UID3);
    sleep(1);
    check_device_connected(UID3);
    port3 = connect_virtual_device("SimpleTestDevice", UID4);
    sleep(1);
    check_device_connected(UID4);

    // print device data
    sleep(1);
    dev_data = get_next_dev_data();
    // check that we see three devices in output
    check_device_sent(dev_data, 0, 62, 13330);
    check_device_sent(dev_data, 1, 62, 9025);
    check_device_sent(dev_data, 2, 62, 16675);
    dev_data__free_unpacked(dev_data, NULL);
    sleep(1);

    // disconnect all devices
    disconnect_virtual_device(port1);
    sleep(1);
    check_device_not_connected(UID3);

    disconnect_virtual_device(port2);
    sleep(1);
    check_device_not_connected(UID2);

    disconnect_virtual_device(port3);
    sleep(1);
    check_device_not_connected(UID4);

    // print device data
    sleep(1);
    dev_data = get_next_dev_data();
    // check that last device data has only the custom data device
    check_device_sent(dev_data, 0, 32, 2020);
    dev_data__free_unpacked(dev_data, NULL);

    return 0;
}
