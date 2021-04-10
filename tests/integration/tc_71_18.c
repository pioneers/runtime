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
char increasing[] = "INCREASING";
char doubling[] = "DOUBLING";
char flipflop[] = "FLIP_FLOP";
char myint[] = "MY_INT";

#define NUM_PARAMS 4

char* parameters[NUM_PARAMS] = {
    increasing,
    doubling,
    flipflop,
    myint,
};

/**
 * Helper function to send dev1_params subscriptions
 * to device 1 and dev2_params subscriptions to device 2
 */
DevData* send_subs(uint32_t dev1_params, uint32_t dev2_params) {
    // send the subscriptions
    dev_subs[0].params = dev1_params;
    dev_subs[1].params = dev2_params;
    send_device_subs(dev_subs, 2);

    // verify that we're receiving only those parameters
    sleep(1);
    return get_next_dev_data();
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
    send_user_input(buttons, joystick_vals, GAMEPAD);

    // verify that we're receiving all parameters
    sleep(1);
    DevData* dev_data = get_next_dev_data();
    check_udp_device_exists(dev_data, 0, 62, 4660);
    check_udp_device_exists(dev_data, 1, 62, 17185);
    check_udp_device_exists(dev_data, 2, 32, 2020);
    for (int i = 0; i < NUM_PARAMS; i++) {
        check_udp_device_param(dev_data, 0, parameters[i], 5, NULL, 5);
        check_udp_device_param(dev_data, 1, parameters[i], 5, NULL, 5);
    }

    // send subs that requests only a few parameters
    dev_data = send_subs(0b11, 0b101);
    check_udp_device_exists(dev_data, 0, 62, 4660);
    check_udp_device_exists(dev_data, 1, 62, 17185);
    check_udp_device_exists(dev_data, 2, 32, 2020);
    check_udp_device_param(dev_data, 0, increasing, 6, NULL, 6);
    check_udp_device_param(dev_data, 0, doubling, 6, NULL, 6);
    check_udp_device_param(dev_data, 1, increasing, 6, NULL, 6);
    check_udp_device_param(dev_data, 1, flipflop, 6, NULL, 6);
    dev_data__free_unpacked(dev_data, NULL);

    // send subs that requests fewer parameters
    dev_data = send_subs(0b1, 0b100);
    check_udp_device_exists(dev_data, 0, 62, 4660);
    check_udp_device_exists(dev_data, 1, 62, 17185);
    check_udp_device_exists(dev_data, 2, 32, 2020);
    check_udp_device_param(dev_data, 0, increasing, 6, NULL, 6);
    check_udp_device_param(dev_data, 1, flipflop, 6, NULL, 6);
    dev_data__free_unpacked(dev_data, NULL);

    dev_data = send_subs(0b11, 0b11);
    check_udp_device_exists(dev_data, 0, 62, 4660);
    check_udp_device_exists(dev_data, 1, 62, 17185);
    check_udp_device_exists(dev_data, 2, 32, 2020);
    check_udp_device_param(dev_data, 0, increasing, 6, NULL, 6);
    check_udp_device_param(dev_data, 0, doubling, 6, NULL, 6);
    check_udp_device_param(dev_data, 1, increasing, 6, NULL, 6);
    check_udp_device_param(dev_data, 1, doubling, 6, NULL, 6);
    dev_data__free_unpacked(dev_data, NULL);

    // send subs that requests the same parameters as last time
    dev_data = send_subs(0b11, 0b11);
    check_udp_device_exists(dev_data, 0, 62, 4660);
    check_udp_device_exists(dev_data, 1, 62, 17185);
    check_udp_device_exists(dev_data, 2, 32, 2020);
    check_udp_device_param(dev_data, 0, increasing, 6, NULL, 6);
    check_udp_device_param(dev_data, 0, doubling, 6, NULL, 6);
    check_udp_device_param(dev_data, 1, increasing, 6, NULL, 6);
    check_udp_device_param(dev_data, 1, doubling, 6, NULL, 6);
    dev_data__free_unpacked(dev_data, NULL);

    // stop the system
    end_test();

    return 0;
}
