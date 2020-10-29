#include "../test.h"

/**
 * This test is a sanity test for testing the testing framework
 * It sends the system into Auto mode and sets the robot to the Right starting position
 * and checks that those actions are reflected in shared memory.
 */

char start_right[] =
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
    start_test("sanity", "", "");

    // poke the system
    send_run_mode(SHEPHERD, AUTO);
    check_run_mode(AUTO);
    send_start_pos(SHEPHERD, RIGHT);
    print_shm();

    // stop the system
    end_test();

    // do checks
    in_rest_of_output(start_right);
    return 0;
}
