#include "../test.h"

/**
 * This test is a sanity test for testing the testing framework
 * It sends the system into Auto mode and sets the robot to the Right starting position
 * and checks that those actions are reflected in shared memory.
 */

int main() {
    start_test("sanity", "", "", NO_REGEX);

    // Verify run mode
    send_run_mode(SHEPHERD, AUTO);
    check_run_mode(AUTO);

    // Verify start position
    send_start_pos(SHEPHERD, RIGHT);
    check_start_pos(RIGHT);

    return 0;
}
