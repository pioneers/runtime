#ifndef TEST_H
#define TEST_H

#include "client/net_handler_client.h"
#include "client/shm_client.h"
#include "client/executor_client.h"
//#include "client/dev_handler_client.h" //TODO: Uncomment this when dev_handler_client is done!

// ****************************************** START/END TEST *************************************** //

/*
 * Takes care of setting up the plumbing for the test output to run.
 * Should be the first function called in EVERY test!
 * Arguments:
 *    - char *test_description: a short description of what the test is testing
 * No return value.
 */
void start_test (char *test_name);

/*
 * Takes care of resetting the plumbing of the outputs at the end of the test, and
 * prepares internal variables for calling the output comparison functions below.
 * Should be called after the test has called all stop_<process>() functions
 */
void end_test ();

// *************************************** OUTPUT COMPARISON FUNCTIONS ***************************** //

/*
 * Function returns true (1) if expected_output is somewhere in the output of the test
 * Arguments:
 *    - char *expected_output: string that should be in the output of the test
 * No return value. (Will exit with status code 1 if not in output).
 */
void in_output (char *expected_output);

/*
 * Function returns true if expected output is somewhere in the output after most recent call to this function
 * Arguments:
 *    - char *expected_output: string that should be in the output of the test AFTER most recent  call to this function
 * No return value. (Will exit with status code 1 if not in rest of output).
 */
void in_rest_of_output (char *expected_output);

/*
 * Function returns true if not_expected_output is not in the output of the test
 * Arguments:
 *    - char *not_expected_output: string that should NOT be anywhere in the output of the test
 * No return value. (Will exit with status code 1 if it found the string in the output).
 */
void not_in_output (char *not_expected_output);

/*
 * Function returns true if not_expected_output is not in the output of the test after most recent call to in_rest_of_output
 * Arguments:
 *    - char *not_expected_output: string that should NOT be anywhere in the output after most recent call to in_rest_of_output
 * No return value. (Will exit with status code 1 if it found the string in the rest of the output).
 */
void not_in_rest_of_output (char *not_expected_output);

#endif
