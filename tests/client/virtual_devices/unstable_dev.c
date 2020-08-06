/**
 * UnstableTestDevice, a virtual device that, at first, acts like a proper
 * lowcar device. However, UnstableTestDevice sleeps after completing
 * a set number of actions, which should trigger dev handler to register
 * a timeout
 */

#include "virtual_device_util.h"

#define MAX_ACTIONS 2

// UnstableTestDevice params
enum {
    NUM_ACTIONS = 0
};

/**
 * Initialize the values for each param
 * Arguments:
 *    params: Array of params to be initialized
 */
void init_params(param_val_t params[]) {
    params[NUM_ACTIONS].p_i = 0;
}

/**
 * Changes device's read-only params
 * Increments NUM_ACTIONS each time, but after it reaches
 * MAX_ACTIONS, sleep forever.
 * Arguments:
 *    params: Array of param values to be modified
 */
void device_actions(param_val_t params[]) {
    if (params[NUM_ACTIONS].p_i >= MAX_ACTIONS) {
        while (1) {
            sleep(60);
        }
    }
    params[NUM_ACTIONS].p_i++;
}

/**
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

    uint8_t dev_type = 61;
    device_t *dev = get_device(dev_type);

    param_val_t params[dev->num_params];
    init_params(params);

    lowcar_protocol(fd, dev_type, dev_type, uid, params, &device_actions);
    return 0;
}
