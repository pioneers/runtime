#include "../test.h"

/**
 * This test is a sanity test for testing the testing framework
 * It sends the system into Auto mode and sets the robot to the Right starting position
 * and checks that those actions are reflected in shared memory.
 */

#define ORDERED_STRINGS 1
#define UNORDERED_STRINGS 0

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
    start_test("sanity", "", "", ORDERED_STRINGS, UNORDERED_STRINGS);

    // poke the system
    send_run_mode(SHEPHERD, AUTO);
    check_run_mode(AUTO);
    send_start_pos(SHEPHERD, RIGHT);
    print_shm();
    add_ordered_string_output(start_right);
    // stop the system
    end_test();


    return 0;
}
