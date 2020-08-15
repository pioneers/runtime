/**
 * Verifies that executor properly writes values to a device
 */

#include "../test.h"

// The UID of GeneralTestDevice to connect and reference in sanity_write.py
#define UID 0x0123456789ABCDEF

int main() {
    // Setup
    start_test("Sanity Write");
    start_shm();
    start_net_handler();
    start_dev_handler();
    start_executor("sanity_write");
    sleep(1);

    // Connect GeneralTestDevice
    connect_virtual_device("GeneralTestDevice", UID);
    sleep(1);   // Wait for ACK exchange

    // Get current parameters then wait
    device_t *dev = get_device(device_name_to_type("GeneralTestDevice"));
    param_val_t initial_vals[dev->num_params];
    device_read_uid(UID, EXECUTOR, DATA, (uint32_t) -1, initial_vals);
    sleep(1);   // Device values will change in this time

    // Start autonomous mode
    send_run_mode(SHEPHERD, AUTO);
    sleep(2); // Executor will write once

    // Get current parameters (ORANGE_FLOAT should be written to)
    param_val_t vals_post_auto[dev->num_params];
    device_read_uid(UID, EXECUTOR, DATA, (uint32_t) -1, vals_post_auto);

    // Wait for executor to write to params again
    sleep(2);
    param_val_t vals_post_second_write[dev->num_params];
    device_read_uid(UID, EXECUTOR, DATA, (uint32_t) -1, vals_post_second_write);

    // Clean up
    disconnect_all_devices();
    stop_executor();
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
    end_test();

    /* Check output in initial_vals, vals_post_auto, and vals_post_second_write
     * The values that sanity_write.py writes to are RED_INT, ORANGE_FLOAT, and YELLOW_BOOL
     * RED_INT initializes to 1, is written to as 1, then is incremented by 2 thereafter
     * ORANGE_FLOAT initializes to 7.7, is written to as 1.0, then is incremented by 3.14 thereafter
     * YELLOW_BOOL initializes to TRUE, is written to as TRUE, then is flipped thereafter
     */
    uint8_t dev_type = device_name_to_type("GeneralTestDevice");
    int red_int_idx = get_param_idx(dev_type, "RED_INT");
    int orange_float_idx = get_param_idx(dev_type, "ORANGE_FLOAT");
    int yellow_bool_idx = get_param_idx(dev_type, "YELLOW_BOOL");
    param_val_t int_one = {.p_i = 1};
    param_val_t float_one = {.p_f = 1.0};
    param_val_t bool_true = {.p_b = 1};
    param_val_t init_orange_float = {.p_f = 7.7};
    // Check initial values are correct
    same_param_value("RED_INT", INT, int_one, initial_vals[red_int_idx]);
    same_param_value("ORANGE_FLOAT", FLOAT, init_orange_float, initial_vals[orange_float_idx]);
    same_param_value("YELLOW_BOOL", BOOL, bool_true, initial_vals[yellow_bool_idx]);
    // Check values after set to autonomous (first executor write). Only ORANGE_FLOAT should change of the three
    same_param_value("RED_INT", INT, int_one, vals_post_auto[red_int_idx]);
    same_param_value("ORANGE_FLOAT", FLOAT, float_one, vals_post_auto[orange_float_idx]);
    same_param_value("YELLOW_BOOL", BOOL, bool_true, vals_post_auto[yellow_bool_idx]);
    // Check values after executor writes again
    param_val_t red_int_post_write_2 = {.p_i = 3};
    param_val_t orange_float_post_write_2 = {.p_f = 4.14};
    param_val_t yellow_bool_post_write_2 = {.p_b = 0};
    same_param_value("RED_INT", INT, red_int_post_write_2, vals_post_second_write[red_int_idx]);
    same_param_value("ORANGE_FLOAT", FLOAT, orange_float_post_write_2, vals_post_second_write[orange_float_idx]);
    same_param_value("YELLOW_BOOL", BOOL, yellow_bool_post_write_2, vals_post_second_write[yellow_bool_idx]);

    return 0;
}
