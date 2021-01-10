#include "test.h"

// The number of characters in a delimiter ("******* STARTING TEST *******")
#define DELIMITER_WIDTH 80
// The maximum length of a string per string check
#define MAX_STRING_LENGTH 128
// Initial capacity of string checks per test
#define BASE_STRING_CHECKS 8

pthread_t output_handler_tid;   // holds thread ID of output handler thread
int save_std_out;               // saved standard output file descriptor to return to normal printing after test
int pipe_fd[2];                 // read and write ends of the pipe created between the parent and child processes
char* test_output = NULL;       // holds the contents of temp file
char* rest_of_test_output;      // holds the rest of the test output as we're performing checks
int check_num = 0;              // increments to report status each time a check is performed
char* global_test_name = NULL;  // name of test
int started_executor = 0;       // boolean of whether or not executor is used in the current test

// String checks
char** ordered_strings_to_check = NULL;    // holds strings needing to be checked in a specified order
char** unordered_strings_to_check = NULL;  // holds strings needing to be checked in any order
int current_ordered_pos = 0;               // current position in ordered_string_to_check pointer
int current_unordered_pos = 0;             // current position in unordered_strings_to_check
int max_ordered_strings = 0;               // maximum amount of ordered strings to check for a given test
int max_unordered_strings = 0;             // maximum amount of unordered strings to check for a given test
int method = NO_REGEX;                     // whether to use standard checking or regex for output comparison

/**
 * This thread prints output to terminal and also copies it to standard output
 * It constantly reads from the read end of the pipe, and then prints it to stderr
 * and stores in test_output. It is canceled by the test in the function end_test().
 * Arguments:
 *    args: always NULL
 * Returns: NULL
 */
static void* output_handler(void* args) {
    size_t curr_size = MAX_LOG_LEN;
    size_t num_total_bytes_read = 0;
    char nextline[MAX_LOG_LEN];
    char* curr_ptr = malloc(curr_size);
    if (curr_ptr == NULL) {
        printf("output_handler: Failed to malloc\n");
        exit(1);
    }
    test_output = curr_ptr;

    // loops until it is canceled
    while (1) {
        fgets(nextline, MAX_LOG_LEN, stdin);  // get the next line from stdin (attached to read end of pipe, which is attached to stdout)
        fprintf(stderr, "%s", nextline);      // print to standard error (attached to terminal)

        // copy the newly read string into curr_ptr
        strcpy(curr_ptr, nextline);

        // increment both num_total_bytes_read and curr_ptr by how long this last line was
        num_total_bytes_read += strlen(curr_ptr);
        curr_ptr += strlen(curr_ptr);

        // double the length of test_output if necessary
        if (curr_size - num_total_bytes_read <= MAX_LOG_LEN) {
            test_output = realloc(test_output, curr_size * 2);
            if (test_output == NULL) {
                fprintf(stderr, "output_handler: Failed to realloc\n");
                exit(1);
            }
            curr_size *= 2;
            curr_ptr = test_output + num_total_bytes_read;
        }
    }
    return NULL;  // never reached
}

/**
 * Prints a delimiter with clean formatting
 * Ex: Input string "Expected:" will print
 * ********** Expected: **********
 *
 * Arguments:
 *    stream: Where to print (stdout/stderr)
 *    format: String format to put in delimter
 */
static void fprintf_delimiter(FILE* stream, char* format, ...) {
    // Build string
    char str[DELIMITER_WIDTH];
    va_list args;
    va_start(args, format);
    vsprintf(str, format, args);
    va_end(args);

    int chars_remaining = DELIMITER_WIDTH;
    const int num_spaces = 2;
    const int num_stars = chars_remaining - strlen(str) - num_spaces;

    // Print stars on left
    for (int i = 0; i < (num_stars / 2); i++) {
        fprintf(stream, "*");
        chars_remaining--;
    }
    // Print string
    fprintf(stream, " %s ", str);
    chars_remaining -= strlen(str) + num_spaces;

    // Print stars on right
    for (int i = 0; i < chars_remaining; i++) {
        fprintf(stream, "*");
    }
    fprintf(stream, "\n");
}

/**
 * Helper function to start up runtime
 */
static void start_runtime(char* student_code, char* challenge_code) {
    start_shm();
    start_net_handler();
    start_dev_handler();
    // If student_code is nonempty or challenge_code is nonempty, start executor
    if (strcmp(student_code, "") != 0 || strcmp(challenge_code, "") != 0) {
        start_executor(student_code, challenge_code);
        started_executor = 1;
    }
    // Make sure runtime starts up
    sleep(1);
}

/**
 * Helper function to stop runtime
 */
static void stop_runtime() {
    if (started_executor) {
        stop_executor();
        started_executor = 0;
    }
    disconnect_all_devices();
    sleep(1);
    stop_dev_handler();
    stop_net_handler();
    stop_shm();
}

// *************************** PASS/FAIL CONTROL **************************** //

// Prints to stderr that a check passed.
static void print_pass() {
    check_num++;
    fprintf(stderr, "%s: check %d passed\n", global_test_name, check_num);
}

// Prints to stderr that a check failed.
static void print_fail() {
    check_num++;
    fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
}

// ******************** STRING OUTPUT COMPARISON HELPERS ******************** //

/**
 * Implementation of strstr but with a regex string for needle
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
static char* rstrstr(char* haystack, char* needle) {
    regex_t expr;
    regmatch_t tracker;

    //Compiles regex string into a usable regex expression
    if (regcomp(&expr, needle, REG_EXTENDED)) {
        fprintf(stderr, "Unable to build regex expression");
        return NULL;
    }
    //Looks for first instance of regex expression in HAYSTACK
    if (regexec(&expr, haystack, 1, &tracker, REG_EXTENDED)) {
        return NULL;
    }
    return haystack + tracker.rm_so;
}

/**
 * Verifies that a string is somewhere in stdout.
 * Arguments:
 *    expected_output: the string to check
 * Returns:
 *    0 if the string is in stdout,
 *    1 otherwise
 */
static int in_output(char* expected_output) {
    if (method == NO_REGEX) {
        if (strstr(test_output, expected_output) != NULL) {
            print_pass();
            return 0;
        }
    } else {
        if (rstrstr(test_output, expected_output) != NULL) {
            print_pass();
            return 0;
        }
    }
    fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
    fprintf_delimiter(stderr, "Expected:");
    fprintf(stderr, "%s", expected_output);
    fprintf_delimiter(stderr, "Got:");
    fprintf(stderr, "%s\n", test_output);
    return 1;
}

/**
 * Verifies that a string is somewhere in stdout ***after the last call to
 * this function***
 *
 * Ex: If output contains "A", "B", "C" and we call in_rest_of_output("B")
 * The function will return true (start scanning from the beginning)
 * If we then call in_rest_of_output("A"), the function will return FALSE.
 * i.e. There is no "A" after "B".
 *
 * Arguments:
 *    expected_output: the string to check
 * Returns:
 *    0 if the string is in stdout,
 *    1 otherwise
 */
static int in_rest_of_output(char* expected_output) {
    // do not use regex
    if (method == NO_REGEX) {
        if ((rest_of_test_output = strstr(rest_of_test_output, expected_output)) != NULL) {
            print_pass();
            rest_of_test_output += strlen(expected_output);
            return 0;
        }
    } else {
        if ((rest_of_test_output = rstrstr(rest_of_test_output, expected_output)) != NULL) {
            print_pass();
            rest_of_test_output += strlen(expected_output);
            return 0;
        }
    }
    fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
    fprintf_delimiter(stderr, "Expected:");
    fprintf(stderr, "%s", expected_output);
    fprintf_delimiter(stderr, "Got:");
    fprintf(stderr, "%s\n", test_output);
    return 1;
}

// Checks that the strings in ordered_strings_to_check and unordered_strings_to_check match the output
static void check_strings() {
    int check_failed = 0;

    // checks the strings in ordered_strings_to_check
    for (int i = 0; i < current_ordered_pos; i++) {
        check_failed |= in_rest_of_output(ordered_strings_to_check[i]);
    }

    // checks the strings in unordered_strings_to_check
    for (int i = 0; i < current_unordered_pos; i++) {
        check_failed |= in_output(unordered_strings_to_check[i]);
    }

    // free memory allocated for ordered_strings_to_check strings and reset variables
    if (ordered_strings_to_check != NULL) {
        for (int i = 0; i < current_unordered_pos; i++) {
            free(ordered_strings_to_check[i]);
        }
        free(ordered_strings_to_check);
        ordered_strings_to_check = NULL;
        current_ordered_pos = 0;
        max_ordered_strings = 0;
    }

    // free memory allocated for unordered_strings_to_check strings and reset variables
    if (unordered_strings_to_check != NULL) {
        for (int i = 0; i < current_unordered_pos; i++) {
            free(unordered_strings_to_check[i]);
        }
        free(unordered_strings_to_check);
        unordered_strings_to_check = NULL;
        current_unordered_pos = 0;
        max_unordered_strings = 0;
    }

    method = NO_REGEX;

    // Exit with exit code 1 if any single check failed
    if (check_failed) {
        exit(1);
    }
}

// ******************** PUBLIC TEST FRAMEWORK FUNCTIONS ********************* //

// creates a pipe to route stdout and stdin to for output handling, spawns the output handler thread
void start_test(char* test_description, char* student_code, char* challenge_code, int comparison_method) {
    fprintf_delimiter(stdout, "Starting Test: \"%s\"", test_description);
    fflush(stdout);

    // save the test name
    global_test_name = malloc(strlen(test_description) + 1);
    if (global_test_name == NULL) {
        printf("start_test: Failed to malloc\n");
        exit(1);
    }
    strcpy(global_test_name, test_description);

    // general string checks needed
    ordered_strings_to_check = malloc(BASE_STRING_CHECKS * sizeof(char*));
    current_ordered_pos = 0;
    if (ordered_strings_to_check == NULL) {
        fprintf(stderr, "strings to check: %s\n", strerror(errno));
    }
    max_ordered_strings = BASE_STRING_CHECKS;

    unordered_strings_to_check = malloc(BASE_STRING_CHECKS * sizeof(char*));
    current_unordered_pos = 0;
    if (unordered_strings_to_check == NULL) {
        fprintf(stderr, "strings to check: %s\n", strerror(errno));
    }
    max_unordered_strings = BASE_STRING_CHECKS;

    // use regex to compare outputs
    if (comparison_method == REGEX) {
        method = comparison_method;
    }

    // create a pipe
    if (pipe(pipe_fd) < 0) {
        fprintf(stderr, "pipe: %s\n", strerror(errno));
    }
    // save the standard out attached to the terminal for later
    save_std_out = dup(fileno(stdout));

    // route read end of pipe to stdin
    if (dup2(pipe_fd[0], fileno(stdin)) == -1) {
        fprintf(stderr, "dup2 stdin to read end of pipe: %s\n", strerror(errno));
    }
    // route write end of pipe to stdout
    if (dup2(pipe_fd[1], fileno(stdout)) == -1) {
        fprintf(stderr, "dup2 stdout to write end of pipe: %s\n", strerror(errno));
    }

    // create the output handler thread
    int status;
    if ((status = pthread_create(&output_handler_tid, NULL, output_handler, NULL)) != 0) {
        fprintf(stderr, "pthread_create: output handler thread: %s\n", strerror(status));
    }

    // Start up runtime
    start_runtime(student_code, challenge_code);
}

void end_test() {
    // Stop runtime
    stop_runtime();

    // cancel the output handler thread
    if (pthread_cancel(output_handler_tid) != 0) {
        fprintf(stderr, "pthread_cancel: output handler thread\n");
    }

    // pull the standard output back to the file descriptor that we saved earlier
    if (dup2(save_std_out, fileno(stdout)) == -1) {
        fprintf(stderr, "dup2 stdout back to terminal: %s\n", strerror(errno));
    }
    // now we are back to normal output

    // set the rest_of_test_output to beginning of test_output to be ready for output comparison
    rest_of_test_output = test_output;
    // we can use printf now and this will go the terminal
    fprintf_delimiter(stderr, "Running Remaining Checks...");

    // check out array of strings
    check_strings();
}

// ******************* STRING OUTPUT COMPARISON FUNCTIONS ******************* //

// Adds the string to be verified to the ordered_strings_to_check pointer
void add_ordered_string_output(char* output) {
    // Adding more strings than the max amount stated
    if (current_ordered_pos >= max_ordered_strings) {
        ordered_strings_to_check = realloc(ordered_strings_to_check, 2 * max_ordered_strings * sizeof(char*));
        if (ordered_strings_to_check == NULL) {
            fprintf(stderr, "Resizing ordered_strings_to_check failed");
            return;
        }
        max_ordered_strings = 2 * max_ordered_strings;
    }
    ordered_strings_to_check[current_ordered_pos] = (char*) malloc(strlen(output) + 1);

    if (ordered_strings_to_check[current_ordered_pos] == NULL) {
        fprintf(stderr, "add ordered output: %s\n", strerror(errno));
        return;
    }

    // copy string into the current position in array and move pointer forward
    strcpy((ordered_strings_to_check[current_ordered_pos]), output);
    current_ordered_pos += 1;
}

// Adds the string to be verified to the unordered_strings_to_check pointer
void add_unordered_string_output(char* output) {
    // Adding more strings than the max amount stated
    if (current_unordered_pos >= max_unordered_strings) {
        unordered_strings_to_check = realloc(unordered_strings_to_check, 2 * max_unordered_strings * sizeof(char*));
        if (unordered_strings_to_check == NULL) {
            fprintf(stderr, "Resizing add_unordered_string_output failed");
            return;
        }
    }

    unordered_strings_to_check[current_unordered_pos] = (char*) malloc(strlen(output) + 1);

    if ((unordered_strings_to_check[current_unordered_pos]) == NULL) {
        fprintf(stderr, "add unordered output: %s\n", strerror(errno));
        return;
    }

    // copy string into the current position in array and move pointer forward
    strcpy((unordered_strings_to_check[current_unordered_pos]), output);
    current_unordered_pos += 1;
}

// ************************* GAMEPAD CHECK FUNCTIONS ************************ //

// Helper function to print a 32-bitmap to stderr on a new line
static void print_bitmap(uint64_t bitmap) {
    uint8_t bit;
    for (int i = 0; i < 64; i++) {
        bit = ((bitmap & (1 << i)) == 0) ? 0 : 1;
        fprintf(stderr, "%d", bit);
    }
    fprintf(stderr, "\n");
}

// Helper function to print joystick values to stderr, each on a new line
static void print_joysticks(float joystick_vals[4]) {
    fprintf(stderr, "JOYSTICK_LEFT_X: %f\n", joystick_vals[JOYSTICK_LEFT_X]);
    fprintf(stderr, "JOYSTICK_LEFT_Y: %f\n", joystick_vals[JOYSTICK_LEFT_Y]);
    fprintf(stderr, "JOYSTICK_RIGHT_X: %f\n", joystick_vals[JOYSTICK_RIGHT_X]);
    fprintf(stderr, "JOYSTICK_RIGHT_Y: %f\n", joystick_vals[JOYSTICK_RIGHT_Y]);
}

void check_inputs(uint64_t expected_buttons, float expected_joysticks[4], robot_desc_field_t source) {
    uint64_t pressed_buttons;
    float joystick_vals[4];
    // Read in the current gamepad state
    if (input_read(&pressed_buttons, joystick_vals, source) != 0) {
        print_fail();
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "%s is not connected\n", field_to_string(source));
        end_test();
        exit(1);
    }
    // Verify that the buttons are correct
    if (pressed_buttons != expected_buttons) {
        print_fail();
        // Print expected button bitmap to stderr
        fprintf_delimiter(stderr, "Expected Pressed Buttons:");
        print_bitmap(expected_buttons);
        // Print current button bitmap to stderr
        fprintf_delimiter(stderr, "Got:");
        print_bitmap(pressed_buttons);
        end_test();
        exit(1);
    }
    // Verify that the joysticks are correct
    if (source == GAMEPAD) {
        for (int i = 0; i < 4; i++) {
            // Fail on the first incorrect joystick value encountered
            if (joystick_vals[i] != expected_joysticks[i]) {
                print_fail();
                // Print all expected joysticks to stderr
                fprintf_delimiter(stderr, "Expected Joysticks:");
                print_joysticks(joystick_vals);
                // Print all current joysticks to stderr
                fprintf_delimiter(stderr, "Got:");
                print_joysticks(joystick_vals);
                end_test();
                exit(1);
            }
        }
    }
    print_pass();
}

// ***************************** RUN MODE CHECK ***************************** //

// Helper function to print out the run mode on a new line to stderr
static void print_run_mode(robot_desc_val_t run_mode) {
    switch (run_mode) {
        case IDLE:
            fprintf(stderr, "IDLE\n");
            break;
        case AUTO:
            fprintf(stderr, "AUTO\n");
            break;
        case TELEOP:
            fprintf(stderr, "TELEOP\n");
            break;
        case CHALLENGE:
            fprintf(stderr, "CHALLENGE\n");
            break;
        default:
            fprintf(stderr, "INVALID RUN MODE\n");
            break;
    }
}

void check_run_mode(robot_desc_val_t expected_run_mode) {
    // Read current run mode
    robot_desc_val_t curr_run_mode = robot_desc_read(RUN_MODE);
    if (curr_run_mode != expected_run_mode) {
        print_fail();
        fprintf_delimiter(stderr, "Expected Run Mode:");
        print_run_mode(expected_run_mode);
        fprintf_delimiter(stderr, "Got:");
        print_run_mode(curr_run_mode);
        end_test();
        exit(1);
    }
    print_pass();
}

// ************************* DEVICE CHECK FUNCTIONS ************************* //

/**
 * Helper function to check whether a specified device is connected
 * Arguments:
 *    dev_uid: the uid of the device
 * Returns:
 *    1 if the device is connected
 *    0 if the device is not connected
 */
static int check_device_helper(uint64_t dev_uid) {
    dev_id_t dev_ids[MAX_DEVICES];
    uint32_t catalog;

    get_device_identifiers(dev_ids);
    get_catalog(&catalog);

    // There are devices connected
    if (catalog != 0) {
        // Iterate through devices
        for (int i = 0; i < MAX_DEVICES; i++) {
            if (catalog & (1 << i)) {  // Connected device at index i
                if (dev_ids[i].uid == dev_uid) {
                    // Device is connected!
                    return 1;
                }
            }
        }
    }
    return 0;
}

void check_device_connected(uint64_t dev_uid) {
    if (check_device_helper(dev_uid) == 1) {
        print_pass();
    } else {
        print_fail();
        fprintf_delimiter(stderr, "Expected:");
        fprintf(stderr, "Connected device (0x%016llX)\n", dev_uid);
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "NOT connected device (0x%016llX)\n", dev_uid);
        end_test();
        exit(1);
    }
}

void check_device_not_connected(uint64_t dev_uid) {
    if (check_device_helper(dev_uid) == 0) {
        print_pass();
    } else {
        print_fail();
        fprintf_delimiter(stderr, "Expected:");
        fprintf(stderr, "NOT connected device (0x%016llX)\n", dev_uid);
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "Connected device (0x%016llX)\n", dev_uid);
        end_test();
        exit(1);
    }
}

// Returns if arrays are the same. Otherwise, exit(1)
void same_param_value_array(uint8_t dev_type, uint64_t uid, param_val_t expected[]) {
    device_t* dev = get_device(dev_type);
    for (int i = 0; i < dev->num_params; i++) {
        same_param_value(dev->name, uid, dev->params[i].name, dev->params[i].type, expected[i]);
    }
}

// Returns if input params are the same. Otherwise, exit(1). Calls check_param_range
void same_param_value(char* dev_name, uint64_t uid, char* param_name, param_type_t param_type, param_val_t expected) {
    check_param_range(dev_name, uid, param_name, param_type, expected, expected);
}

// Returns if value is between the expected high and expected low
void check_param_range(char* dev_name, uint64_t uid, char* param_name, param_type_t param_type, param_val_t expected_low, param_val_t expected_high) {
    uint8_t dev_type = device_name_to_type(dev_name);
    device_t* dev = get_device(dev_type);

    param_val_t vals_after[dev->num_params];
    uint32_t param_idx = (uint32_t) get_param_idx(dev_type, param_name);
    device_read_uid(uid, EXECUTOR, DATA, (1 << param_idx), vals_after);
    param_val_t received = vals_after[param_idx];

    switch (param_type) {
        case INT:
            if (received.p_i < expected_low.p_i || received.p_i > expected_high.p_i) {
                print_fail();
                fprintf_delimiter(stderr, "Expected:");
                fprintf(stderr, "%s >= %d\n", param_name, expected_low.p_i);
                fprintf(stderr, "%s <= %d\n", param_name, expected_high.p_i);
                fprintf_delimiter(stderr, "Got:");
                fprintf(stderr, "%s == %d\n", param_name, received.p_i);
                end_test();
                exit(1);
            }
            break;
        case FLOAT:
            if (received.p_f < expected_low.p_f || received.p_f > expected_high.p_f) {
                print_fail();
                fprintf_delimiter(stderr, "Expected:");
                fprintf(stderr, "%s >= %f\n", param_name, expected_low.p_f);
                fprintf(stderr, "%s <= %f\n", param_name, expected_high.p_f);
                fprintf_delimiter(stderr, "Got:");
                fprintf(stderr, "%s == %f\n", param_name, received.p_f);
                end_test();
                exit(1);
            }
            break;
        case BOOL:
            // Invalid if low != high and fail if received != low (or high)
            if (expected_low.p_b != expected_high.p_b || received.p_b != expected_low.p_b) {
                print_fail();
                fprintf_delimiter(stderr, "Expected:");
                fprintf(stderr, "Param range check of type INT or FLOAT\n");
                fprintf_delimiter(stderr, "Got:");
                fprintf(stderr, "Param range check of type BOOL\n");
                end_test();
                exit(1);
            }
    }
    print_pass();
}
