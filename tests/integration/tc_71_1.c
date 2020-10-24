#include "../test.h"

/**
 * This test is a sanity test for testing the testing framework
 * It sends the system into Auto mode and sets the robot to the Right starting position
 * and checks that those actions are reflected in shared memory.
 */

char check_1_output[] =
    "Changed devices: 00000000000000000000000000000000\n"
    "Changed params:\n"
    "Requested devices: 00000000000000000000000000000000\n"
    "Requested params:\n"
    "Current Robot Description:\n"
    "\tRUN_MODE = AUTO\n"
    "\tDAWN = CONNECTED\n"
    "\tSHEPHERD = CONNECTED\n"
    "\tGAMEPAD = DISCONNECTED\n"
    "\tSTART_POS = LEFT\n\n"
    "Current Gamepad State:\n"
    "\tNo gamepad currently connected\n";

char check_2_output[] =
    "Changed devices: 00000000000000000000000000000000\n"
    "Changed params:\n"
    "Requested devices: 00000000000000000000000000000000\n"
    "Requested params:\n"
    "Current Robot Description:\n"
    "\tRUN_MODE = AUTO\n"
    "\tDAWN = CONNECTED\n"
    "\tSHEPHERD = CONNECTED\n"
    "\tGAMEPAD = DISCONNECTED\n"
    "\tSTART_POS = RIGHT\n\n"
    "Current Gamepad State:\n"
    "\tNo gamepad currently connected\n";

int main() {
    // setup
    start_test("sanity");
    start_shm();
    start_net_handler();

    // poke the system
    send_run_mode(SHEPHERD, AUTO);
    print_shm();
    send_start_pos(SHEPHERD, RIGHT);
    print_shm();

    // stop the system
    stop_net_handler();
    stop_shm();
    end_test();

    // do checks
    in_rest_of_output(check_1_output, NO_REGEX);
    in_rest_of_output(check_2_output, NO_REGEX);
    return 0;
}
