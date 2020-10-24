#include "test.h"

#define DELIMITER_WIDTH 80

pthread_t output_handler_tid;   // holds thread ID of output handler thread
int save_std_out;               // saved standard output file descriptor to return to normal printing after test
int pipe_fd[2];                 // read and write ends of the pipe created between the parent and child processes
char* test_output = NULL;       // holds the contents of temp file
char* rest_of_test_output;      // holds the rest of the test output as we're performing checks
int check_num = 0;              // increments to report status each time a check is performed
char* global_test_name = NULL;  // name of test

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

// ******************** PUBLIC TEST FRAMEWORK FUNCTIONS ********************* //

// creates a pipe to route stdout and stdin to for output handling, spawns the output handler thread
void start_test(char* test_description) {
    fprintf_delimiter(stdout, "Starting Test: \"%s\"", test_description);
    fflush(stdout);

    // save the test name
    global_test_name = malloc(strlen(test_description) + 1);
    strcpy(global_test_name, test_description);

    // create a pipe
    if (pipe(pipe_fd) < 0) {
        printf("pipe: %s\n", strerror(errno));
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
}

// this is called when the test has shut down all runtime processes and is ready to compare output
// cancels the output handling thread and resets output
void end_test() {
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
    fprintf_delimiter(stderr, "Running Checks...");
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

// ******************* STRING OUTPUT COMPARISON FUNCTIONS ******************* //

// Verifies that expected_output is somewhere in the output
void in_output(char* expected_output) {
    if (strstr(rest_of_test_output, expected_output) != NULL) {
        print_pass();
        return;
    } else {
        print_fail();
        fprintf_delimiter(stderr, "Expected:");
        fprintf(stderr, "%s", expected_output);
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "%s\n", test_output);
        exit(1);
    }
}

// Verifies that expected_output is somewhere in the output after the last call to in_rest_of_output
void in_rest_of_output(char* expected_output) {
    if ((rest_of_test_output = strstr(rest_of_test_output, expected_output)) != NULL) {
        print_pass();
        rest_of_test_output += strlen(expected_output);  // advance rest_of_test_output past what we were looking for
        return;
    } else {
        print_fail();
        fprintf_delimiter(stderr, "Expected:");
        fprintf(stderr, "%s", expected_output);
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "%s\n", test_output);
        exit(1);
    }
}

// Verifies that not_expected_output is not anywhere in the output
void not_in_output(char* not_expected_output) {
    if (strstr(test_output, not_expected_output) == NULL) {
        print_pass();
        return;
    } else {
        print_fail();
        fprintf_delimiter(stderr, "Not Expected:");
        fprintf(stderr, "%s", not_expected_output);
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "%s\n", test_output);
        exit(1);
    }
}

// Verifies that not_expected_output is not anywhere in the output after the last call to in_rest_of_output
void not_in_rest_of_output(char* not_expected_output) {
    if (strstr(rest_of_test_output, not_expected_output) == NULL) {
        print_pass();
        return;
    } else {
        print_fail();
        fprintf_delimiter(stderr, "Not Expected:");
        fprintf(stderr, "%s", not_expected_output);
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "%s\n", test_output);
        exit(1);
    }
}

// ************************* GAMEPAD CHECK FUNCTIONS ************************ //

// Helper function to print a 32-bitmap to stderr on a new line
static void print_bitmap(uint32_t bitmap) {
    uint8_t bit;
    for (int i = 0; i < 32; i++) {
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

void check_gamepad(uint32_t expected_buttons, float expected_joysticks[4]) {
    uint32_t pressed_buttons;
    float joystick_vals[4];
    // Read in the current gamepad state
    if (gamepad_read(&pressed_buttons, joystick_vals) != 0) {
        print_fail();
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "%s\n", "Gamepad is not connected");
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
        exit(1);
    }
    // Verify that the joysticks are correct
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
            exit(1);
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
        case AUTO:
            fprintf(stderr, "AUTO\n");
        case TELEOP:
            fprintf(stderr, "TELEOP\n");
        case CHALLENGE:
            fprintf(stderr, "CHALLENGE\n");
        default:
            fprintf(stderr, "INVALID RUN MODE\n");
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
            if (catalog & (1 << i)) { // Connected device at index i
                if (dev_ids[i].uid == dev_uid) {
                    // Device is connected!
                    return 1;
                }
            }
        }
    }
    return 0;
}

void check_device_connected(uint8_t dev_type, uint64_t dev_uid) {
    if (check_device_helper(dev_uid) == 1) {
        print_pass();
    } else {
        print_fail();
        fprintf_delimiter(stderr, "Expected:");
        fprintf(stderr, "Connected %s (0x%016llX)\n", get_device_name(dev_type), dev_uid);
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "NOT connected %s (0x%016llX)\n", get_device_name(dev_type), dev_uid);
        exit(1);
    }
}

void check_device_not_connected(uint8_t dev_type, uint64_t dev_uid) {
    if (check_device_helper(dev_uid) == 0) {
        print_pass();
    } else {
        print_fail();
        fprintf_delimiter(stderr, "Expected:");
        fprintf(stderr, "NOT Connected %s (0x%016llX)\n", get_device_name(dev_type), dev_uid);
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "Connected %s (0x%016llX)\n", get_device_name(dev_type), dev_uid);
        exit(1);
    }
}

// Returns if arrays are the same. Otherwise, exit(1)
void same_param_value_array(uint8_t dev_type, param_val_t expected[], param_val_t received[]) {
    device_t* dev = get_device(dev_type);
    for (int i = 0; i < dev->num_params; i++) {
        same_param_value(dev->params[i].name, dev->params[i].type, expected[i], received[i]);
    }
}

// Returns if input params are the same. Otherwise, exit(1)
void same_param_value(char* param_name, param_type_t param_type, param_val_t expected, param_val_t received) {
    switch (param_type) {
        case INT:
            if (expected.p_i != received.p_i) {
                print_fail();
                fprintf_delimiter(stderr, "Expected:");
                fprintf(stderr, "%s == %d\n", param_name, expected.p_i);
                fprintf_delimiter(stderr, "Got:");
                fprintf(stderr, "%s == %d\n", param_name, received.p_i);
                exit(1);
            }
            break;
        case FLOAT:
            if (expected.p_f != received.p_f) {
                print_fail();
                fprintf_delimiter(stderr, "Expected:");
                fprintf(stderr, "%s == %f\n", param_name, expected.p_f);
                fprintf_delimiter(stderr, "Got:");
                fprintf(stderr, "%s == %f\n", param_name, received.p_f);
                exit(1);
            }
            break;
        case BOOL:
            if (expected.p_b != received.p_b) {
                print_fail();
                fprintf_delimiter(stderr, "Expected:");
                fprintf(stderr, "%s == %d\n", param_name, expected.p_b);
                fprintf_delimiter(stderr, "Got:");
                fprintf(stderr, "%s == %d\n", param_name, received.p_b);
                exit(1);
            }
            break;
    }
    print_pass();
}
