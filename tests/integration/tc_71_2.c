#include "../test.h"

/**
 * This test is a decently comprehensive executor sanity check (and a demonstration of the test framework's executor-testing abilities)
 * It tests to make sure the testing framework can load student code from our testing folder, runs two coding challenges, does a basic
 * test of Robot.run(), checks that both auto and teleop mode work, and checks that an error is outputted to the screen when a Python
 * exception occurs.
 */

char check_output_6[] =
    "line 25, in teleop_main\n"
    "    oops = 1 / 0\n"
    "ZeroDivisionError: division by zero\n";

int main() {
    // set everything up
    start_test("executor sanity test", "executor_sanity", NO_REGEX);

    // poke the system
    // this section checks the autonomous code (should generate some print statements)
    send_start_pos(SHEPHERD, RIGHT);
    add_ordered_string_output("Autonomous setup has begun!\n");

    // Verify auto mode
    send_run_mode(SHEPHERD, AUTO);
    add_ordered_string_output("autonomous printing again\n");
    sleep(1);
    check_run_mode(AUTO);

    // Verify idle mode
    send_run_mode(SHEPHERD, IDLE);
    sleep(1);
    check_run_mode(IDLE);

    // this section checks the teleop code (should generate division by zero error)
    send_run_mode(SHEPHERD, TELEOP);
    add_ordered_string_output("Traceback (most recent call last):\n");
    add_ordered_string_output(check_output_6);
    add_ordered_string_output("Python function teleop_main call failed\n");
    check_run_mode(TELEOP);

    send_run_mode(DAWN, IDLE);

    return 0;
}
