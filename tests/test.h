#ifndef TEST_H
#define TEST_H

#include <regex.h>

#include "client/dev_handler_client.h"
#include "client/executor_client.h"
#include "client/net_handler_client.h"
#include "client/shm_client.h"

#define NO_REGEX 0
#define REGEX 1

//CAUTION: regex versions of tests assumes
//the input is a regex string to be built.
//Thus, if you want to look for the string (1)
//in 200(1), you must input [(]1[)] instead
//of (1)

// ***************************** START/END TEST ***************************** //

/**
 * Takes care of setting up the plumbing for the test output to run.
 * Should be the first function called in EVERY test!
 * Arguments:
 *    - char *test_description: a short description of what the test is testing
 * No return value.
 */
void start_test(char* test_name);

/**
 * Takes care of resetting the plumbing of the outputs at the end of the test, and
 * prepares internal variables for calling the output comparison functions below.
 * Should be called after the test has called all stop_<process>() functions
 */
void end_test();

/**
 * An implementation of strstr that uses regex
 * Functions exactly the same, but has regex
 * capabilities. If nothing is found in the
 * HAYSTACK, then it will return NULL, otherwise
 * it will return the rest of the HAYSTACK after
 * the first instance of NEEDLE.
 *
 * Remember, it takes in a regex string,
 * so backslashes have to be escaped as \\
 *
 * Example usage:
 * rstrstr("     There are 123456789 chickens here",
 *      "[[:space]:]* There are [0-9]+")
 * returns
 *      " chickens here"
 *
 *
 * Common Regex:
 * [[:space:]]* represents arbitrary whitespace
 * [0-9]+ represents a number
 * [a-z]+ lowercase word
 * [A-Z]+ uppercase word
 * [a-zA-Z]+ alphabetical words
 * [a-zA-Z0-9]+ alphanumeric strings
 * * represents zero or more instances
 * + represents one or more instances
 * "something here" will look for the entire string 'something here' within the haystack
 */

char* rstrstr(char* haystack, char* needle);

// ********************** OUTPUT COMPARISON FUNCTIONS *********************** //

/**
 * Verifies that expected_output is somewhere in the output of the test
 * Arguments:
 *    expected_output: string that should be in the output of the test
 *    use_regex: Whether or not to use regex matching with EXPECTED_OUTPUT
 * No return value. (Will exit with status code 1 if not in output).
 */
void in_output(char* expected_output, int use_regex);

/**
 * Verifies that expected output is somewhere in the output after most recent call to this function
 * Arguments:
 *    expected_output: string that should be in the output of the test AFTER most recent  call to this function
 *    use_regex: Whether or not to use regex matching with EXPECTED_OUTPUT
 * No return value. (Will exit with status code 1 if not in rest of output).
 */
void in_rest_of_output(char* expected_output, int use_regex);

/**
 * Verifies that not_expected_output is not in the output of the test
 * Arguments:
 *    not_expected_output: string that should NOT be anywhere in the output of the test
 *    use_regex: Whether or not to use regex matching with NOT_EXPECTED_OUTPUT
 * No return value. (Will exit with status code 1 if it found the string in the output).
 */
void not_in_output(char* not_expected_output, int use_regex);

/**
 * Verifies that not_expected_output is not in the output of the test after most recent call to in_rest_of_output
 * Arguments:
 *    not_expected_output: string that should NOT be anywhere in the output after most recent call to in_rest_of_output
 *    use_regex: Whether or not to use regex matching with NOT_EXPECTED_OUTPUT
 * No return value. (Will exit with status code 1 if it found the string in the rest of the output).
 */
void not_in_rest_of_output(char* not_expected_output, int use_regex);

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
void same_param_value(char* param_name, param_type_t param_type, param_val_t expected, param_val_t received);

#endif
