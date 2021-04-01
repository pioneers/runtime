#include "../test.h"

/**
 * This test seeks to verify the behavior in Github issue #203
 * If Shepherd is connected, the student should not be able to set the mode to TELEOP or AUTO,
 * i.e. Shepherd should have full control over the robot state. Dawn should, however, be able
 * to set the run mode to IDLE and override Shepherd to emergency-stop the robot.
 */

char check_output_1[] =
    "You cannot send Robot to Auto or Teleop from Dawn with Shepherd connected!";

int main() {
    // set everything up
    start_test("block_dawn_teleop_auto", "studentcode", "studentcode", NO_REGEX);

    // poke the system
    // Set auto mode from Shepherd and verify change
    send_run_mode(SHEPHERD, AUTO);
    sleep(1);
    check_run_mode(AUTO);  // Verify auto mode

    // Try to set teleop mode from Dawn and verify no change and error message
    send_run_mode(DAWN, TELEOP);
    add_ordered_string_output(check_output_1);
    sleep(1);
    check_run_mode(AUTO);

    // Try to set auto mode from Dawn and verify no change and error message
    send_run_mode(DAWN, AUTO);
    add_ordered_string_output(check_output_1);
    sleep(1);
    check_run_mode(AUTO);

    // Try to set idle mode from Dawn and verify mode change
    send_run_mode(DAWN, IDLE);
    sleep(1);
    check_run_mode(IDLE);

    // Set teleop mode from Shepherd and verify change
    send_run_mode(SHEPHERD, TELEOP);
    sleep(1);
    check_run_mode(TELEOP);

    // Try to set teleop mode from Dawn and verify no change and error message
    send_run_mode(DAWN, TELEOP);
    add_ordered_string_output(check_output_1);
    sleep(1);
    check_run_mode(TELEOP);

    // Try to set auto mode from Dawn and verify no change and error message
    send_run_mode(DAWN, AUTO);
    add_ordered_string_output(check_output_1);
    sleep(1);
    check_run_mode(TELEOP);

    // Try to set idle mode from Dawn and verify mode change
    send_run_mode(DAWN, IDLE);
    sleep(1);
    check_run_mode(IDLE);

    end_test();
    return 0;
}
