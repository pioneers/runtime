/**
 * TimeTestDevice, a virtual device that has two parameters:
 * 1) BOOL get_time, and
 * 2) INT timestamp
 * When GET_TIME is set to 1, TIMESTAMP is set to the lower sizeof(INT) bytes
 * of the number of milliseconds since the UNIX epoch
 * GET_TIME is then set back to 0
 *
 * Motivation:
 *    Student Code: When button is pressed, write 1 to GET_TIME
 *    Net handler receives button press and sends it down to the device
 *    When the device receives the signal to write to GET_TIME, it updates TIMESTAMP
 *    Subtract the timestamp of the button press from TIMESTAMP
 *    The difference is the (approximate) latency between net handler's
 *    inception of the gamepad state from Dawn and the actual devices
 */

#include "virtual_device_util.h"

// TimeTestDevice params
enum {
    GET_TIME = 0,
    TIMESTAMP = 1
};

/**
 * Initialize the values for each param
 * Arguments:
 *    params: Array of params to be initialized
 */
void init_params(param_val_t params[]) {
    params[GET_TIME].p_b = 0;
    params[TIMESTAMP].p_i = 0;
}

/**
 * Changes device's read-only params
 * Arguments:
 *    params: Array of param values to be modified
 */
void device_actions(param_val_t params[]) {
    if (params[TIMESTAMP].p_i != 0) {
        // Send only once
        return;
    } else if (params[GET_TIME].p_b) {
        uint64_t time = millis();
        params[TIMESTAMP].p_i = time % 1000000000;  // Send last 9 digits
        params[GET_TIME].p_b = 0;
    }
}

/**
 * Arguments:
 *    int: file descriptor for the socket
 *    uint64_t: device uid
 */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Incorrect number of arguments: %d out of %d\n", argc, 3);
        exit(1);
    }

    int fd = atoi(argv[1]);
    uint64_t uid = strtoull(argv[2], NULL, 0);

    uint8_t dev_type = device_name_to_type("TimeTestDevice");
    device_t* dev = get_device(dev_type);

    param_val_t params[dev->num_params];
    init_params(params);

    lowcar_protocol(fd, dev_type, dev_type, uid, params, &device_actions, 0);
    return 0;
}
