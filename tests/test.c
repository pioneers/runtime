#include "test.h"

#define TEMP_FILE "/tmp/temp_logs.txt"

#define DELIMITER_WIDTH 80

pid_t output_redirect;          // holds process ID of output redirection process
FILE *temp_fp;                  // file pointer of open temp file
int save_std_out;               // saved standard output file descriptor to return to normal printing after test
int pipe_fd[2];                 // read and write ends of the pipe created between the parent and child processes
char *test_output = NULL;       // holds the contents of temp file
char *rest_of_test_output;      // holds the rest of the test output as we're performing checks
int check_num = 0;              // increments to report status each time a check is performed
char *global_test_name = NULL;  // name of test

// child process sigint handler
static void child_sigint_handler (int signum) {
    fclose(temp_fp);   // close temp file
    close(pipe_fd[0]); // close read end of pipe
    exit(0);
}

// child process main function that just allows stdout to go to the temp file and waits for SIGINT
static void child_main_function () {
    char nextline[MAX_LOG_LEN];

    // register sigint handler
    signal(SIGINT, child_sigint_handler);

    // wait for SIGINT to come from parent
    while (1) {
        fgets(nextline, MAX_LOG_LEN, stdin);
        fprintf(stderr, "%s", nextline);      // print to standard error (attached to terminal)
        fprintf(temp_fp, "%s", nextline);     // print to temp file
    }
}

// this function kills and waits for child process
static void kill_child () {
    // flush standard output buffer
    fflush(stdout);

    // kill and wait for the child process
    if (kill(output_redirect, SIGINT) < 0) {
        printf("kill test output redirect: %s\n", strerror(errno));
    }
    if (waitpid(output_redirect, NULL, 0) < 0) {
        printf("waitpid test output redirect: %s\n", strerror(errno));
    }
}

// cleanup done for main process
static void cleanup_handler () {
    free(global_test_name);
    free(test_output);
}

/* Prints a delimiter with clean formatting
 * Ex: Input string "Expected:" will print
 * ********** Expected: **********
 *
 * Arguments:
 *    stream: Where to print (stdout/stderr)
 *    format: String format to put in delimter
 */
static void fprintf_delimiter(FILE *stream, char *format, ...) {
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

// creates a child process that duplicates the output from the test to both standard out and to a temp file
void start_test (char *test_name) {
    fprintf_delimiter(stdout, "Starting test: \"%s\"", test_name);
    fflush(stdout);

    // save the test name
    global_test_name = malloc(strlen(test_name) + 1);
    strcpy(global_test_name, test_name);

    // create a pipe
    if (pipe(pipe_fd) < 0) {
        printf("pipe: %s\n", strerror(errno));
    }

    // fork this process; in the child, route stdin to read end of pipe; in parent, route stdout to write end of pipe
    if ((output_redirect = fork()) < 0) {
        printf("fork: %s\n", strerror(errno));
    } else if (output_redirect == 0) { // child
        // open the temp file
        if ((temp_fp = fopen(TEMP_FILE, "w")) == NULL) {
            printf("open: cannot create temp file: %s\n", strerror(errno));
        }

        // route stdin to read end of pipe
        if (dup2(pipe_fd[0], fileno(stdin)) == -1) {
            fprintf(stderr, "dup2 stdin to read end of pipe: %s\n", strerror(errno));
        }
        close(pipe_fd[1]); // don't need write end

        child_main_function();
    } else { // parent
        // save the standard out attached to the terminal for later
        save_std_out = dup(fileno(stdout));

        // route stdout to write end of pipe
        if (dup2(pipe_fd[1], fileno(stdout)) == -1) {
            fprintf(stderr, "dup2 stdout to write end of pipe: %s\n", strerror(errno));
        }
        close(pipe_fd[0]); // don't need read end

        // kill the child on Ctrl-C (SIGINT)
        signal(SIGINT, kill_child);

        // register the clean up handler
        atexit(cleanup_handler);
    }
}

// this is called when the test has shut down all runtime processes and is ready to compare output
// kills the child process, then reads in the entire contents of the temporary file into a string
void end_test () {
    size_t curr_size = 1024;
    size_t num_total_bytes_read = 0;
    char *curr_ptr = test_output = malloc(curr_size);

    // kill the child process
    kill_child();

    // now pull the standard output back to the file descriptor that we saved earlier
    if (dup2(save_std_out, fileno(stdout)) == -1) {
        fprintf(stderr, "dup2 stdout back to terminal: %s\n", strerror(errno));
    }
    close(save_std_out);
    // now we are back to normal output

    // open the temp file as a file pointer
    FILE *temp_fp = fopen(TEMP_FILE, "r");

    // while we haven't encountered EOF yet, read the next line
    while (fgets(curr_ptr, MAX_LOG_LEN, temp_fp) != NULL) {
        // increment both total bytes read and current pointer by how long the newly read line was
        num_total_bytes_read += strlen(curr_ptr);
        curr_ptr += strlen(curr_ptr);

        // expand test_output to twice as large as before if necessary
        if (curr_size - num_total_bytes_read <= MAX_LOG_LEN) {
            test_output = realloc(test_output, curr_size * 2);
            curr_size *= 2;
            curr_ptr = test_output + num_total_bytes_read;
        }
    }
    rest_of_test_output = test_output;
    fclose(temp_fp);
    remove(TEMP_FILE);
    fprintf_delimiter(stdout, "Running Checks...");
}

// Verifies that expected_output is somewhere in the output
void in_output (char *expected_output) {
    check_num++;
    if (strstr(rest_of_test_output, expected_output) != NULL) {
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
void in_rest_of_output (char *expected_output) {
    check_num++;
    if ((rest_of_test_output = strstr(rest_of_test_output, expected_output)) != NULL) {
        fprintf(stderr, "%s: check %d passed\n", global_test_name, check_num);
        rest_of_test_output += strlen(expected_output); // advance rest_of_test_output past what we were looking for
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
void not_in_output (char *not_expected_output) {
    check_num++;
    if (strstr(test_output, not_expected_output) == NULL) {
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
void not_in_rest_of_output (char *not_expected_output) {
    check_num++;
    if (strstr(rest_of_test_output, not_expected_output) == NULL) {
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
    device_t *dev = get_device(dev_type);
    for (int i = 0; i < dev->num_params; i++) {
        same_param_value(dev->params[i].name, dev->params[i].type, expected[i], received[i]);
    }
}

// Returns if input params are the same. Otherwise, exit(1)
void same_param_value(char *param_name, param_type_t param_type, param_val_t expected, param_val_t received) {
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
