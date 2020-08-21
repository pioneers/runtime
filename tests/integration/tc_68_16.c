#include "../test.h"

/**
 * This tests recreates as closely as possible the tests
 * that were run when we manually tested net_handler with
 * a hard-coded TCP client program and a hard-coded UDP
 * client program. Checks challenge data + student code
 * in different files.
 */

#define UID 0x1234

// checks challenge results
char check_1_output[] =
    "Challenge 0 result: 9302\n"
    "Challenge 1 result: [2, 661, 35963]\n";

// checks initial device data
char check_2_output[] =
    "Device No. 0:\ttype = SimpleTestDevice, uid = 4660, itype = 62\n"
    "\tParams:\n"
    "\t\tparam \"INCREASING\" has type INT with value 2\n"
    "\t\tparam \"DOUBLING\" has type FLOAT with value 4.000000\n"
    "\t\tparam \"FLIP_FLOP\" has type BOOL with value True\n"
    "\t\tparam \"MY_INT\" has type INT with value 1\n"
    "Device No. 1:\ttype = CustomData, uid = 0, itype = 32\n"
    "\tParams:\n"
    "\t\tparam \"time_ms\" has type INT with value";

char check_3_output[] = "Teleop setup has begun!\n";
char check_4_output[] = "a button not pressed!\n";

// checks device data second time
char check_5_output[] =
    "Device No. 0:\ttype = SimpleTestDevice, uid = 4660, itype = 62\n"
    "\tParams:\n"
    "\t\tparam \"INCREASING\" has type INT with value 3\n"
    "\t\tparam \"DOUBLING\" has type FLOAT with value 8.000000\n"
    "\t\tparam \"FLIP_FLOP\" has type BOOL with value False\n"
    "\t\tparam \"MY_INT\" has type INT with value 0\n";

// check for left joystick moving and a button pressed
char check_6_output[] = "Left joystick moved in x direction!\n";
char check_7_output[] = "a button pressed!\n";

// checks device data after pushing button
char check_8_output[] =
    "Device No. 0:\ttype = SimpleTestDevice, uid = 4660, itype = 62\n"
    "\tParams:\n"
    "\t\tparam \"INCREASING\" has type INT with value 6\n"
    "\t\tparam \"DOUBLING\" has type FLOAT with value 64.000000\n"
    "\t\tparam \"FLIP_FLOP\" has type BOOL with value True\n"
    "\t\tparam \"MY_INT\" has type INT with value 1\n";

int main() {
    // setup
    start_test("net_handler integration");
    start_shm();
    start_net_handler();
    start_dev_handler();
    
    // poke the system
    connect_virtual_device("SimpleTestDevice", UID);
    start_executor("net_handler_integration", "challenges_sanity");
    usleep(1000000); // sleep 1.25 seconds to offset device from executor
    
    // send challenge data
	char *inputs[] = { "2039", "190172344" };
	send_challenge_data(DAWN, inputs, 2);
    
    // send gamepad state
	uint32_t buttons = 0;
	float joystick_vals[] = { 0.0, 0.0, 0.0, 0.0 };
    send_gamepad_state(buttons, joystick_vals);
    
    // print device data
    print_next_dev_data();
    
    // put into teleop
    send_run_mode(DAWN, TELEOP);
    
    // print device data
    print_next_dev_data();
    sleep(1);

    // send gamepad state
    buttons = (1 << A_BUTTON) | (1 << DOWN_DPAD);
    joystick_vals[X_LEFT_JOYSTICK] = -0.1;
    send_gamepad_state(buttons, joystick_vals);

    // sleep to let stuff happen
    sleep(1);

    // print device data three times, once per second
    print_next_dev_data();
    sleep(1);
     
    // put back in idle mode
    send_run_mode(DAWN, IDLE);
    
    // stop the system
    disconnect_all_devices();
    stop_dev_handler();
    stop_executor();
    stop_net_handler();
    stop_shm();
    end_test();
    
    // check output
    in_rest_of_output(check_1_output);
    in_rest_of_output(check_2_output);
    in_rest_of_output(check_3_output);
    in_rest_of_output(check_4_output);
    in_rest_of_output(check_5_output);
    for (int i = 0; i < 3; i++) {
        in_rest_of_output(check_6_output);
    }
    in_rest_of_output(check_7_output);
    in_rest_of_output(check_8_output);

    return 0;
}
