/**
 * Verifies that executor properly writes values to a device
 */

#include "../test.h"

// The UID of GeneralTestDevice to connect and reference in sanity_write.py
#define UID 0x0123456789ABCDEF

int main() {
    // Setup
    start_test("Sanity Write", "sanity_write", "", NO_REGEX);

    // Connect GeneralTestDevice
    char* dev_name = "GeneralTestDevice";
    connect_virtual_device(dev_name, UID);
    sleep(1);  // Wait for ACK exchange

    /* Check output in initial_vals, vals_post_auto, and vals_post_second_write
     * The values that sanity_write.py writes to are RED_INT, ORANGE_FLOAT, and YELLOW_BOOL
     * RED_INT initializes to 1, is written to as 1, then is incremented by 2 thereafter
     * ORANGE_FLOAT initializes to 7.7, is written to as 1.0, then is incremented by 3.14 thereafter
     * YELLOW_BOOL initializes to TRUE, is written to as TRUE, then is flipped thereafter
     */

    // Get current parameters then wait
    param_val_t int_one = {.p_i = 1};
    param_val_t float_one = {.p_f = 1.0};
    param_val_t bool_true = {.p_b = 1};
    param_val_t init_orange_float = {.p_f = 7.7};
    // Check initial values are correct
    same_param_value(dev_name, UID, "RED_INT", INT, int_one);
    same_param_value(dev_name, UID, "ORANGE_FLOAT", FLOAT, init_orange_float);
    same_param_value(dev_name, UID, "YELLOW_BOOL", BOOL, bool_true);
    sleep(1);  // Device values will change in this time

    // Start autonomous mode
    send_run_mode(SHEPHERD, AUTO);
    sleep(2);  // Executor will write once

    // Get current parameters (ORANGE_FLOAT should be written to)
    same_param_value(dev_name, UID, "RED_INT", INT, int_one);
    same_param_value(dev_name, UID, "ORANGE_FLOAT", FLOAT, float_one);
    same_param_value(dev_name, UID, "YELLOW_BOOL", BOOL, bool_true);

    // Wait for executor to write to params again
    sleep(2);

    // Check values after executor writes again
    param_val_t red_int_post_write_2 = {.p_i = 3};
    param_val_t orange_float_post_write_2 = {.p_f = 4.14};
    param_val_t yellow_bool_post_write_2 = {.p_b = 0};
    same_param_value(dev_name, UID, "RED_INT", INT, red_int_post_write_2);
    same_param_value(dev_name, UID, "ORANGE_FLOAT", FLOAT, orange_float_post_write_2);
    same_param_value(dev_name, UID, "YELLOW_BOOL", BOOL, yellow_bool_post_write_2);

    return 0;
}
