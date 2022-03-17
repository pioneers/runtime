/**
 * A virtual device that behaves like a proper lowcar device
 */

#include <stdio.h>   // for print
#include <stdlib.h>  // for atoi()

#include "virtual_device_util.h"

// ************************ GENERAL DEVICE SPECIFIC ************************* //

// GeneralTestDevice params
enum {
    // Neither Read nor Write
    CONSTANT_NEITHER,
    // Write-only
    CONSTANT_WRITE,
    // Read-only
    INCREASING_ODD,
    DECREASING_ODD,
    INCREASING_EVEN,
    DECREASING_EVEN,
    INCREASING_FLIP,
    ALWAYS_LEET,
    DOUBLING,
    DOUBLING_NEG,
    HALFING,
    HALFING_NEG,
    EXP_ONE_PT_ONE,
    EXP_ONE_PT_TWO,
    ALWAYS_PI,
    FLIP_FLOP,
    ALWAYS_TRUE,
    ALWAYS_FALSE,
    // Read and Write
    RED_INT,
    ORANGE_INT,
    GREEN_INT,
    BLUE_INT,
    PURPLE_INT,
    RED_FLOAT,
    ORANGE_FLOAT,
    GREEN_FLOAT,
    BLUE_FLOAT,
    PURPLE_FLOAT,
    RED_BOOL,
    ORANGE_BOOL,
    GREEN_BOOL,
    BLUE_BOOL,
    PURPLE_BOOL,
    YELLOW_BOOL
};

/**
 * Initialize the values for each param
 * Arguments:
 *    params: Array of params to be initialized
 */
void init_params(param_val_t params[]) {
    // Neither Read nor Write
    params[CONSTANT_NEITHER].p_i = 1;
    // Write only
    params[CONSTANT_WRITE].p_i = 1;
    // Read only
    // params[INCREASING_ODD].p_i = 1;
    // params[DECREASING_ODD].p_i = -1;
    params[INCREASING_EVEN].p_i = 0;
    params[DECREASING_EVEN].p_i = 0;
    params[INCREASING_FLIP].p_i = 0;
    params[ALWAYS_LEET].p_i = 1337;
    params[DOUBLING].p_f = 1;
    params[DOUBLING_NEG].p_f = -1;
    params[HALFING].p_f = 1048576;  // 2^20
    params[HALFING_NEG].p_f = -1048576;
    params[EXP_ONE_PT_ONE].p_f = 1;
    params[EXP_ONE_PT_TWO].p_f = 1;
    params[ALWAYS_PI].p_f = 3.14159265359;
    params[FLIP_FLOP].p_b = 1;
    params[ALWAYS_TRUE].p_b = 1;
    params[ALWAYS_FALSE].p_b = 0;
    // Read and Write
    params[RED_INT].p_i = 1;
    params[ORANGE_INT].p_i = 2;
    params[GREEN_INT].p_i = 3;
    params[BLUE_INT].p_i = 4;
    params[PURPLE_INT].p_i = 5;
    params[RED_FLOAT].p_f = 6.6;
    params[ORANGE_FLOAT].p_f = 7.7;
    params[GREEN_FLOAT].p_f = 8.8;
    params[BLUE_FLOAT].p_f = 9.9;
    params[PURPLE_FLOAT].p_f = 10.1;
    params[RED_BOOL].p_b = 1;
    params[ORANGE_BOOL].p_b = 1;
    params[GREEN_BOOL].p_b = 1;
    params[BLUE_BOOL].p_b = 1;
    params[PURPLE_BOOL].p_b = 1;
    params[YELLOW_BOOL].p_b = 1;
}

/**
 * Changes device's read-only params
 * Arguments:
 *    params: Array of param values to be modified
 */
void device_actions(param_val_t params[]) {
    // params[INCREASING_ODD].p_i += 2;
    // params[DECREASING_ODD].p_i -= 2;
    params[INCREASING_EVEN].p_i += 2;
    params[DECREASING_EVEN].p_i -= 2;
    params[INCREASING_FLIP].p_i = (-1) *
                                  (params[INCREASING_FLIP].p_i + ((params[INCREASING_FLIP].p_i % 2 == 0) ? 1 : -1));  // -1, 2, -3, 4, etc

    params[DOUBLING].p_f *= 2;
    params[DOUBLING_NEG].p_f *= 2;
    params[HALFING].p_f /= 2;  // 2^20
    params[HALFING_NEG].p_f /= 2;
    params[EXP_ONE_PT_ONE].p_f *= 1.1;
    params[EXP_ONE_PT_TWO].p_f *= 1.2;
    params[FLIP_FLOP].p_b = 1 - params[FLIP_FLOP].p_b;
}

// ********************************* MAIN *********************************** //

/**
 * A device that behaves like a lowcar device, connected to dev handler via a socket
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

    uint8_t dev_type = device_name_to_type("GeneralTestDevice");
    device_t* dev = get_device(dev_type);

    param_val_t params[dev->num_params];
    init_params(params);

    lowcar_protocol(fd, dev_type, dev_type, uid, params, &device_actions, 1000);
    return 0;
}
