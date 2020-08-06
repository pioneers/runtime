/**
 * SimpleTestDevice, A virtual device that acts like a proper lowcar device
 * The same as GeneralTestDevice except with only 4 parameters
 */
#include "virtual_device_util.h"

// SimpleTestDevice params
enum {
    // Read-only
    INCREASING,
    DOUBLING,
    FLIP_FLOP,
    // Read and Write
    MY_INT
};

/**
 * Initialize the values for each param
 * Arguments:
 *    params: Array of params to be initialized
 */
void init_params(param_val_t params[]) {
    params[INCREASING].p_i = 0;
    params[DOUBLING].p_f = 1;
    params[FLIP_FLOP].p_b = 1;
    params[MY_INT].p_i = 1;
}

/**
 * Changes device's read-only params
 * Arguments:
 *    params: Array of param values to be modified
 */
void device_actions(param_val_t params[]) {
    params[INCREASING].p_i += 1;
    params[DOUBLING].p_f *= 2;
    params[FLIP_FLOP].p_b = 1 - params[FLIP_FLOP].p_b;
}

/**
 * A device that behaves like a lowcar device, connected to dev handler via a socket
 * Arguments:
 *    int: file descriptor for the socket
 *    uint64_t: device uid
 */
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Incorrect number of arguments: %d out of %d\n", argc, 3);
        exit(1);
    }

    int fd = atoi(argv[1]);
    uint64_t uid = strtoull(argv[2], NULL, 0);

    uint8_t dev_type = 62;
    device_t *dev = get_device(dev_type);

    param_val_t params[dev->num_params];
    init_params(params);

    lowcar_protocol(fd, dev_type, dev_type, uid, params, &device_actions);
    return 0;
}
