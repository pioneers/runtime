#include "../test.h"

/**
 * This test exercises the device subscription feature of Runtime
 * through net_handler. It verifies that the default of all parameters
 * subscribed is overriden by a sending of device data, among other things.
 */

#define UID1 0x1234
#define UID2 0x4321

// global variables to hold device subscriptions
dev_subs_t dev1_subs = {UID1, "SimpleTestDevice", 0};
dev_subs_t dev2_subs = {UID2, "SimpleTestDevice", 0};
dev_subs_t dev_subs[2];

// check that certain names of parameters appear

// headers for devices
char dev1_header[] = "Device No. 0:\ttype = SimpleTestDevice, uid = 4660, itype = 62\n";
char dev2_header[] = "Device No. 1:\ttype = SimpleTestDevice, uid = 17185, itype = 62\n";
char custom_dev_header[] = "Device No. 2:\ttype = CustomData, uid = 2020, itype = 32\n";

// parameters names we look for
char increasing[] = "\"INCREASING\"";
char doubling[] = "\"DOUBLING\"";
char flipflop[] = "\"FLIP_FLOP\"";
char myint[] = "\"MY_INT\"";

/**
 * Helper function to send dev1_params subscriptions
 * to device 1 and dev2_params subscriptions to device 2
 */
void send_subs(uint32_t dev1_params, uint32_t dev2_params) {
    // send the subscriptions
    dev_subs[0].params = dev1_params;
    dev_subs[1].params = dev2_params;
    send_device_subs(dev_subs, 2);

    // verify that we're receiving only those parameters
    sleep(1);
    print_next_dev_data();
}

int main() {
    // setup
    start_test("device subscription test", "", "", NO_REGEX);
    dev_subs[0] = dev1_subs;
    dev_subs[1] = dev2_subs;

    // connect two devices
    connect_virtual_device("SimpleTestDevice", UID1);
    connect_virtual_device("SimpleTestDevice", UID2);

    // send gamepad state so net_handler starts sending device data packets
    uint32_t buttons = 0;
    float joystick_vals[] = {0.0, 0.0, 0.0, 0.0};
    send_gamepad_state(buttons, joystick_vals);

    // verify that we're receiving all parameters
    sleep(1);
    print_next_dev_data();
    add_ordered_string_output(dev1_header);
    add_ordered_string_output(increasing);
    add_ordered_string_output(doubling);
    add_ordered_string_output(flipflop);
    add_ordered_string_output(myint);
    add_ordered_string_output(dev2_header);
    add_ordered_string_output(increasing);
    add_ordered_string_output(doubling);
    add_ordered_string_output(flipflop);
    add_ordered_string_output(myint);
    add_ordered_string_output(custom_dev_header);
    // send subs that requests only a few parameters
    send_subs(0b11, 0b101);
    add_ordered_string_output(dev1_header);
    add_ordered_string_output(increasing);
    add_ordered_string_output(doubling);
    add_ordered_string_output(dev2_header);
    add_ordered_string_output(increasing);
    add_ordered_string_output(flipflop);
    add_ordered_string_output(custom_dev_header);

    // send subs that requests fewer parameters
    send_subs(0b1, 0b100);
    add_ordered_string_output(dev1_header);
    add_ordered_string_output(increasing);
    add_ordered_string_output(dev2_header);
    add_ordered_string_output(flipflop);
    add_ordered_string_output(custom_dev_header);

    send_subs(0b11, 0b11);
    add_ordered_string_output(dev1_header);
    add_ordered_string_output(increasing);
    add_ordered_string_output(doubling);
    add_ordered_string_output(dev2_header);
    add_ordered_string_output(increasing);
    add_ordered_string_output(doubling);
    add_ordered_string_output(custom_dev_header);

    // send subs that requests the same parameters as last time
    send_subs(0b11, 0b11);
    add_ordered_string_output(dev1_header);
    add_ordered_string_output(increasing);
    add_ordered_string_output(doubling);
    add_ordered_string_output(dev2_header);
    add_ordered_string_output(increasing);
    add_ordered_string_output(doubling);
    add_ordered_string_output(custom_dev_header);
    // stop the system
    end_test();

    return 0;
}
