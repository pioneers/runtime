/**
 * Functions to start, stop, and conduct tests.
 * Includes (but not limited to) string output matching, device parameter
 * checks, user input checks, and what devices are connected and aren't connected.
 *
 * All tests must begin with start_test()
 * The other functions defined in this header file can be used to verify the
 * state of runtime.
 */
#ifndef TEST_H
#define TEST_H

#include <regex.h>
#include <stdbool.h>

#include "client/dev_handler_client.h"
#include "client/executor_client.h"
#include "client/net_handler_client.h"
#include "client/shm_client.h"

#define NO_REGEX 0
#define REGEX 1

/**
 * CAUTION: regex versions of tests assumes the input is a regex string to be built.
 * Ex: If you want to look for the sub-string "(1)" in "200(1)"
 *     You must input "[(]1[)]" or "\\(1\\)" instead of "(1)"
 */

// ***************************** START/END TEST ***************************** //

/**
 * Takes care of setting up the plumbing for the test output to run.
 * Also starts up runtime (shm, net handler, dev handler, executor)
 * Should be the first function called in EVERY test!
 * Arguments:
 *    test_name: a short description of what the test is testing
 *    student_code: python file name of student code, excluding ".py"
 *    string_checks: number of strings needing to be checked for this test
 *    comparison_method: Whether to use regex for all string matching
 *      in this test (NO_REGEX or REGEX)
 * No return value.
 */
void start_test(char* test_description, char* student_code, int comparison_method);

// ******************* STRING OUTPUT COMPRISON FUNCTIONS ******************** //

/**
 * Add output from test to the ordered_string_output array.
 * Arguments:
 *    output: the expected bitmap of pressed buttons
 * No return value.
 */
void add_ordered_string_output(char* output);

/**
 * Add output from test to the unordered_string_output array.
 * Arguments:
 *    output: the expected bitmap of pressed buttons
 * No return value.
 */
void add_unordered_string_output(char* output);

// ************************* USER INPUT CHECK FUNCTIONS ************************ //

/**
 * Verifies that the the state of the inputs is as expected in shared memory.
 * Arguments:
 *    expected_buttons: the expected bitmap of pressed buttons
 *    expected_joysticks: the expected joystick values
 *                        (Use gp_joystick_t enum in runtime util for indexing)
 *    source: which input source to check against, either GAMEPAD or KEYBOARD
 * No return value. (Will exit with status code 1 if incorrect input state)
 */
void check_inputs(uint64_t expected_buttons, float expected_joysticks[4], robot_desc_field_t source);

/******************** UDP Device Data Checks ******************/

/**
 * Checks that the given device was sent by the UDP thread.
 * 
 * Args:
 *  dev_data: Protobuf struct that is outputted by the UDP thread from get_next_device_data()
 *  index: index where device should be
 *  type: what type the device should be
 *  uid: what uid the device should be
 * 
 */
void check_device_sent(DevData* dev_data, int index, uint8_t type, uint64_t uid);

/**
 * Checks that the device at the given index has a parameter with the given attributes.
 * 
 * Args:
 *  dev_data: Protobuf struct that is outputted by the UDP thread from get_next_device_data()
 *  index: index where device should be
 *  param_name: name of parameter to check
 *  param_type: desired type of parameter
 *      If it is not one of INT, FLOAT, BOOL, then we assume that we don't care about checking parameter value.
 *  param_val: desired value of parameter, only used if param_type is valid. Otherwise, use NULL
 *  readonly: whether the parameter should be readonly. If it is not 0 or 1, then readonly isn't checked.
 * 
 */
void check_device_param_sent(DevData* dev_data, int dev_idx, char* param_name, param_type_t param_type, param_val_t* param_val, uint8_t readonly);

// ***************************** RUN MODE CHECK ***************************** //

/**
 * Verifies that the current robot run mode is as expected in shared memory.
 * Arguments:
 *    expected_run_mode: the expected run mode.
 * No return value. (Will exit with status code 1 if incorrect run mode)
 */
void check_run_mode(robot_desc_val_t expected_run_mode);

// ***************************** START POS CHECK ***************************** //

/**
 * Verifies that the current robot start position is as expected in shared memory.
 * Arguments:
 *    expected_start_pos: the expected start position.
 * No return value. (Will exit with status code 1 if incorrect start pos)
 */
void check_start_pos(robot_desc_val_t expected_start_pos);

// *************************** SUBSCRIPTION CHECK **************************** //

/**
 * Verifies that the current subscriptions are as expected in shared memory.
 * Arguments:
 *    dev_uid: the device uid to check subscriptions on
 *    expected_sub_map: the expected bitmap of parameter subscriptions
 *    process: the process that is expected to have made the aforementioned subscriptions
 *             Choose from: NET_HANDLER, EXECUTOR, or TEST
 *             Set as TEST if it doesn't matter who made the subscription.
 * No return value. (Will exit with status code 1 if incorrect subscriptions)
 */
void check_sub_requests(uint64_t dev_uid, uint32_t expected_sub_map, process_t process);

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
void same_param_value_array(uint8_t dev_type, uint64_t uid, param_val_t expected[]);

/**
 * Verifies that the device's specified parameter is the same as the expected parameter
 * Calls check_param_range where expected_low == expected_high == expected
 * Arguments:
 *    dev_name: The name of the device whose parameters are being checked
 *    uid: uniquie identifier of device
 *    param_name: The name of the parameter being compared
 *    param_type: The data type being compared
 *    expected: The parameter value expected from a test
 * Returns nothing if they're the same.
 * Exits with status code 1 if they're different
 */
void same_param_value(char* dev_name, uint64_t uid, char* param_name, param_type_t param_type, param_val_t expected);

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
void check_param_range(char* dev_name, uint64_t uid, char* param_name, param_type_t param_type, param_val_t expected_low, param_val_t expected_high);

/**
* Checks the latency between an action and the TIMESTAMP parameter on a TimeTestDevice
 * Arguments:
 *    uid: unique identifier of TimeTestDevice
 *    upper_bound_latency: The expected upperbound latency
 *    start_time: The start of a timer provided by the test case(in ms), usually a call to the millis() function
*/
void check_latency(uint64_t uid, int32_t upper_bound_latency, uint64_t start_time);
#endif
