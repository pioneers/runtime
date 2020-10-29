/**
 * Functions to start, stop, and conduct tests.
 * Includes (but not limited to) string output matching, device parameter
 * checks, gamepad checks, and what devices are connected and aren't connected.
 */
#ifndef TEST_H
#define TEST_H

#include "client/dev_handler_client.h"
#include "client/executor_client.h"
#include "client/net_handler_client.h"
#include "client/shm_client.h"

// ***************************** START/END TEST ***************************** //

/**
 * Takes care of setting up the plumbing for the test output to run.
 * Also starts up runtime (shm, net handler, dev handler, executor)
 * Should be the first function called in EVERY test!
 * Arguments:
 *    test_name: a short description of what the test is testing
 *    student_code: python file name of student code, excluding ".py"
 *    challenge_code: python file name of challenge code, excluding ".py"
 *    ** Set both STUDENT_CODE and CHALLENGE_CODE to empty string if executor
 *      is not necessary in the test.
 * No return value.
 */
void start_test(char* test_name, char* student_code, char* challenge_code);

/**
 * Stops runtime, takes care of resetting the plumbing of the outputs at the
 * end of the test, and prepares internal variables for calling the output
 * comparison functions below.
 */
void end_test();

// ******************* STRING OUTPUT COMPARISON FUNCTIONS ******************* //

/**
 * Verifies that expected_output is somewhere in the output of the test
 * Arguments:
 *    expected_output: string that should be in the output of the test
 * No return value. (Will exit with status code 1 if not in output).
 */
void in_output(char* expected_output);

/**
 * Verifies that expected output is somewhere in the output after most recent call to this function
 * Arguments:
 *    expected_output: string that should be in the output of the test AFTER most recent  call to this function
 * No return value. (Will exit with status code 1 if not in rest of output).
 */
void in_rest_of_output(char* expected_output);

/**
 * Verifies that not_expected_output is not in the output of the test
 * Arguments:
 *    not_expected_output: string that should NOT be anywhere in the output of the test
 * No return value. (Will exit with status code 1 if it found the string in the output).
 */
void not_in_output(char* not_expected_output);

/**
 * Verifies that not_expected_output is not in the output of the test after most recent call to in_rest_of_output
 * Arguments:
 *    not_expected_output: string that should NOT be anywhere in the output after most recent call to in_rest_of_output
 * No return value. (Will exit with status code 1 if it found the string in the rest of the output).
 */
void not_in_rest_of_output(char* not_expected_output);

// ************************* GAMEPAD CHECK FUNCTIONS ************************ //

/**
 * Verifies that the the state of the gamepad is as expected in shared memory.
 * Arguments:
 *    expected_buttons: the expected bitmap of pressed buttons
 *    expected_joysticks: the expected joystick values
 *                        (Use gp_joystick_t enum in runtime util for indexing)
 * No return value. (Will exit with status code 1 if incorrect gamepad state)
 */
void check_gamepad(uint32_t expected_buttons, float expected_joysticks[4]);

// ***************************** RUN MODE CHECK ***************************** //

/**
 * Verifies that the current robot run mode is as expected in shared memory.
 * Arguments:
 *    expected_run_mode: the expected run mode.
 * No return value. (Will exit with status code 1 if incorrect run mode)
 */
void check_run_mode(robot_desc_val_t expected_run_mode);

// ************************* DEVICE CHECK FUNCTIONS ************************* //

/**
 * Verifies that the specified device is connected.
 * If the device is NOT connected, the test fails.
 * Arguments:
 *    dev_uid: the device uid
 */
void check_device_connected(uint64_t dev_uid);

/**
 * Verifies that the specified device is NOT connected.
 * If the device IS connected, the test fails.
 * Arguments:
 *    dev_uid: the device uid
 */
void check_device_not_connected(uint64_t dev_uid);

/**
 * Verifies that two input arrays of parameters are the same
 * Arguments:
 *    dev_type: The type of the device the parameters are for
 *    expected: Array of parameter values expected from a test
 *    received: Array of parameter values received from a test
 * Returns nothing if they're the same.
 * Exits with status code 1 if they're different
 */
void same_param_value_array(uint8_t dev_type, uint64_t UID, param_val_t expected[]);

/**
 * Verifies that the device's specified parameter is the same as the expected parameter
 * Arguments:
 *    dev_name: The name of the device whose parameters are being checked
 *    uid: uniquie identifier of device 
 *    param_name: The name of the parameter being compared
 *    param_type: The data type being compared
 *    expected: The parameter value expected from a test
 * Returns nothing if they're the same.
 * Exits with status code 1 if they're different
 */
void same_param_value(char* dev_name, uint64_t UID, char* param_name, param_type_t param_type, param_val_t expected) ;

/**
 * Same as above but compares the received value to a range of expected values
 * Arguments:
 *    dev_name: The name of the device whose parameters are being checked
 *    uid: uniquie identifier of device 
 *    param_name: The name of the parameter being compared
 *    param_type: The data type being compared
 *    expected_low: The lowerbound expected value
 *    expected_high: The upperbound expect value
 * Returns nothing if within range
 * Exits with status code 1 if out of range
 */
void check_param_range(char* dev_name , uint64_t UID, char* param_name, param_type_t param_type, param_val_t expected_low, param_val_t expected_high);

#endif
