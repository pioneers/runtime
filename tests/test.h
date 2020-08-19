#ifndef TEST_H
#define TEST_H

#include "client/net_handler_client.h"
#include "client/shm_client.h"
#include "client/executor_client.h"
#include "client/dev_handler_client.h"

// ***************************** START/END TEST ***************************** //

/**
 * Takes care of setting up the plumbing for the test output to run.
 * Should be the first function called in EVERY test!
 * Arguments:
 *    - char *test_description: a short description of what the test is testing
 * No return value.
 */
void start_test(char *test_name);

/**
 * Takes care of resetting the plumbing of the outputs at the end of the test, and
 * prepares internal variables for calling the output comparison functions below.
 * Should be called after the test has called all stop_<process>() functions
 */
void end_test();

// ********************** OUTPUT COMPARISON FUNCTIONS *********************** //

/**
 * Verifies that expected_output is somewhere in the output of the test
 * Arguments:
 *    expected_output: string that should be in the output of the test
 * No return value. (Will exit with status code 1 if not in output).
 */
void in_output(char *expected_output);

/**
 * Verifies that expected output is somewhere in the output after most recent call to this function
 * Arguments:
 *    expected_output: string that should be in the output of the test AFTER most recent  call to this function
 * No return value. (Will exit with status code 1 if not in rest of output).
 */
void in_rest_of_output(char *expected_output);

/**
 * Verifies that not_expected_output is not in the output of the test
 * Arguments:
 *    not_expected_output: string that should NOT be anywhere in the output of the test
 * No return value. (Will exit with status code 1 if it found the string in the output).
 */
void not_in_output(char *not_expected_output);

/**
 * Verifies that not_expected_output is not in the output of the test after most recent call to in_rest_of_output
 * Arguments:
 *    not_expected_output: string that should NOT be anywhere in the output after most recent call to in_rest_of_output
 * No return value. (Will exit with status code 1 if it found the string in the rest of the output).
 */
void not_in_rest_of_output(char *not_expected_output);

/**
 * Verifies that two input arrays of parameters are the same
 * Arguments:
 *    dev_type: The type of the device the parameters are for
 *    expected: Array of parameter values expected from a test
 *    received: Array of parameter values received from a test
 * Returns nothing if they're the same.
 * Exits with status code 1 if they're different
 */
void same_param_value_array(uint8_t dev_type, param_val_t expected[], param_val_t received[]);

/**
 * Verifies that two parameters have the same value
 * Arguments:
 *    param_name: The name of the parameter being compared
 *    param_type: The data type being compared
 *    expected: The parameter value expected from a test
 *    received: The parameter value received from a test
 * Returns nothing if they're the same.
 * Exits with status code 1 if they're different
 */
void same_param_value(char *param_name, param_type_t param_type, param_val_t expected, param_val_t received);

#endif
