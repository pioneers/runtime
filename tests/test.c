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

// ********************************************** PUBLIC TEST FRAMEWORK FUNCTIONS ************************************** //

// creates a pipe to route stdout and stdin to for output handling, spawns the output handler thread
void start_test(char* test_description) {
    printf("************************************** Starting test: \"%s\" **************************************\n", test_description);
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
    printf("************************************** Running Checks... ******************************************\n");
}

//Implementation of strstr but with a regex string for needle
char* rstrstr(const char* haystack, const char* needle) {
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

// Verifies that expected_output is somewhere in the output
void in_output(char* expected_output, int USE_REGEX) {
    char* check;

    check_num++;
    if (USE_REGEX == NO_REGEX) {
        check = strstr(test_output, expected_output);
    } else {
        check = rstrstr(test_output, expected_output);
    }
    if (check != NULL) {
        fprintf(stderr, "%s: check %d passed\n", global_test_name, check_num);
        return;
    } else {
        fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
        fprintf_delimiter(stderr, "Expected:");
        fprintf(stderr, "%s", expected_output);
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "%s\n", test_output);
        exit(1);
    }
}

// Verifies that expected_output is somewhere in the output after the last call to in_rest_of_output
void in_rest_of_output(char* expected_output, int USE_REGEX) {
    char* check;

    check_num++;
    if (USE_REGEX == NO_REGEX) {
        rest_of_test_output = strstr(rest_of_test_output, expected_output);
    } else {
        rest_of_test_output = rstrstr(rest_of_test_output, expected_output);
    }
    if (rest_of_test_output != NULL) {
        fprintf(stderr, "%s: check %d passed\n", global_test_name, check_num);
        rest_of_test_output += strlen(expected_output);  // advance rest_of_test_output past what we were looking for
        return;
    } else {
        fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
        fprintf(stderr, "********************************** Expected: ***********************************\n");
        fprintf(stderr, "%s", expected_output);
        fprintf(stderr, "************************************* Got: *************************************\n");
        fprintf(stderr, "%s\n", test_output);
        exit(1);
    }
}

// Verifies that not_expected_output is not anywhere in the output
void not_in_output(char* not_expected_output, int USE_REGEX) {
    char* check;

    check_num++;
    if (USE_REGEX == NO_REGEX) {
        check = strstr(test_output, not_expected_output);
    } else {
        check = rstrstr(test_output, not_expected_output);
    }
    if (check == NULL) {
        fprintf(stderr, "%s: check %d passed\n", global_test_name, check_num);
        return;
    } else {
        fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
        fprintf_delimiter(stderr, "Not Expected:");
        fprintf(stderr, "%s", not_expected_output);
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "%s\n", test_output);
        exit(1);
    }
}

// Verifies that not_expected_output is not anywhere in the output after the last call to in_rest_of_output
void not_in_rest_of_output(char* not_expected_output, int USE_REGEX) {
    char* check;

    check_num++;
    if (USE_REGEX == NO_REGEX) {
        check = strstr(rest_of_test_output, not_expected_output);
    } else {
        check = rstrstr(rest_of_test_output, not_expected_output);
    }
    if (check == NULL) {
        fprintf(stderr, "%s: check %d passed\n", global_test_name, check_num);
        return;
    } else {
        fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
        fprintf_delimiter(stderr, "Not Expected:");
        fprintf(stderr, "%s", not_expected_output);
        fprintf_delimiter(stderr, "Got:");
        fprintf(stderr, "%s\n", test_output);
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
    check_num++;
    switch (param_type) {
        case INT:
            if (expected.p_i != received.p_i) {
                fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
                fprintf_delimiter(stderr, "Expected:");
                fprintf(stderr, "%s == %d\n", param_name, expected.p_i);
                fprintf_delimiter(stderr, "Got:");
                fprintf(stderr, "%s == %d\n", param_name, received.p_i);
                exit(1);
            }
            break;
        case FLOAT:
            if (expected.p_f != received.p_f) {
                fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
                fprintf_delimiter(stderr, "Expected:");
                fprintf(stderr, "%s == %f\n", param_name, expected.p_f);
                fprintf_delimiter(stderr, "Got:");
                fprintf(stderr, "%s == %f\n", param_name, received.p_f);
                exit(1);
            }
            break;
        case BOOL:
            if (expected.p_b != received.p_b) {
                fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
                fprintf_delimiter(stderr, "Expected:");
                fprintf(stderr, "%s == %d\n", param_name, expected.p_b);
                fprintf_delimiter(stderr, "Got:");
                fprintf(stderr, "%s == %d\n", param_name, received.p_b);
                exit(1);
            }
            break;
    }
    fprintf(stderr, "%s: check %d passed\n", global_test_name, check_num);
}
