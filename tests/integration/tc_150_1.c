#include "../test.h"

/**
 * Tests the Keyboard input source through net_handler, SHM, and executor. 
 */

int main() {
    start_test("keyboard_input", "keyboard_input", "", NO_REGEX);

    connect_virtual_device("SimpleTestDevice", 20);
    
    float garbage[4];
    uint64_t buttons = 1 << 25; // 'z'
    send_user_input(buttons, garbage, KEYBOARD);
    sleep(.1);
    check_inputs(buttons, garbage, KEYBOARD);

    send_run_mode(DAWN, TELEOP);
    sleep(.1);

    buttons = 1 << 24; // 'y'
    send_user_input(buttons, garbage, KEYBOARD);
    sleep(.1);
    check_inputs(buttons, garbage, KEYBOARD);
    sleep(1);
    add_ordered_string_output("1");
    add_ordered_string_output("1");

    buttons = 1; // 'a'
    send_user_input(buttons, garbage, KEYBOARD);
    sleep(1);
    check_inputs(buttons, garbage, KEYBOARD);
    sleep(.2);

    param_val_t expected = {.p_i = 123454321};
    same_param_value("SimpleTestDevice", 20, "MY_INT", INT, expected);

    end_test();
    return 0;
}