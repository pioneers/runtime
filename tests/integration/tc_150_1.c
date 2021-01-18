#include "../test.h"

/**
 * Tests the Keyboard input source through net_handler, SHM, and executor. 
 */

int main() {
    start_test("keyboard_input", "keyboard_input", "", NO_REGEX);

    connect_virtual_device("SimpleTestDevice", 20);
    sleep(1);
    float garbage[4];
    uint64_t buttons = get_key_bit("z");
    send_user_input(buttons, garbage, KEYBOARD);
    sleep(1);
    check_inputs(buttons, garbage, KEYBOARD);

    send_run_mode(DAWN, TELEOP);
    sleep(.1);

    buttons = get_key_bit("y");
    send_user_input(buttons, garbage, KEYBOARD);
    sleep(.8);
    check_inputs(buttons, garbage, KEYBOARD);
    sleep(1);

    // Check that student code is run properly which will print out the current value of MY_INT twice, which should be 1
    add_ordered_string_output(") 1\n");
    add_ordered_string_output(") 1\n");

    buttons = get_key_bit("a");
    send_user_input(buttons, garbage, KEYBOARD);
    sleep(1);
    check_inputs(buttons, garbage, KEYBOARD);
    sleep(.2);

    param_val_t expected = {.p_i = 123454321};
    same_param_value("SimpleTestDevice", 20, "MY_INT", INT, expected);

    end_test();
    return 0;
}
