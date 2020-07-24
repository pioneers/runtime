#include "test.h"

#define TEMP_FILE "temp.txt"

char *test_output = NULL;       //holds the contents of temp file
int temp_fd;                    //file descriptor of open temp file
int save_stdout;                //saved file descriptor of standard output
int check_num = 0;              //increments to report status each time a check is performed
char *global_test_name = NULL;  //name of test

//removes the temporary file on exit
static void cleanup_handler ()
{
	close(temp_fd);
	remove(TEMP_FILE);
	free(global_test_name);
	free(test_output);
}

//routes stdout of test process to a temporary file to collect output from all the clients
void start_test (char *test_name)
{
    printf("************************************** Starting test: \"%s\" **************************************\n", test_name);
    fflush(stdout);

	//save the test name
	global_test_name = malloc(strlen(test_name));
	strcpy(global_test_name, test_name);
	
	//open the temp file
	if ((temp_fd = open(TEMP_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
		printf("open: cannot create temp file: %s\n", strerror(errno));
	}
   
    //save standard output descriptor to save_stdout
    save_stdout = dup(fileno(stdout));

	//route standard output to this file
	if (dup2(temp_fd, fileno(stdout)) == -1) {
		fprintf(stderr, "dup2 stdout to temp_fd: %s\n", strerror(errno));
	}
	atexit(cleanup_handler);
}

//this is called when the test has shut down all runtime processes and is ready to compare output
//reads in the entire contents of the temporary file into a string
void end_test ()
{
	size_t curr_size = 1024;
	size_t num_total_bytes_read = 0;
	char *curr_ptr = test_output = malloc(curr_size);

    //flush standard output buffer (which is still attached to the temp file)
    fflush(stdout);

    //now pull the standard output back to the file descriptor that we saved earlier
    if (dup2(save_stdout, fileno(stdout)) == -1) {
        fprintf(stderr, "dup2 stdout to terminal: %s\n", strerror(errno));
    }
    close(save_stdout);
    //now we are back to normal output

    //open the temp file as a file pointer
    close(temp_fd);
    FILE *temp_fp = fopen(TEMP_FILE, "r");

	//while we haven't encountered EOF yet, read the next line
	while (fgets(curr_ptr, MAX_LOG_LEN, temp_fp) != NULL) {
		//increment both total bytes read and current pointer by how long the newly read line was
		num_total_bytes_read += strlen(curr_ptr);
		curr_ptr += strlen(curr_ptr);
		
		//expand test_output to twice as large as before if necessary
		if (curr_size - num_total_bytes_read <= MAX_LOG_LEN) {
			test_output = realloc(test_output, curr_size * 2);
			curr_size *= 2;
			curr_ptr = test_output + num_total_bytes_read;
		}
	}
    printf("************************************** Output of test: \"%s\" *************************************\n\n%s\n\n", global_test_name, test_output);
    printf("************************************** Running Checks... ******************************************\n");
}

//returns true if exepcted output matches output exactly
int match_all (char *expected_output)
{
	check_num++;
	if (strcmp(test_output, expected_output) == 0) {
		fprintf(stderr, "%s: check %d passed\n", global_test_name, check_num);
		return 0;
	} else {
		fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
		fprintf(stderr, "************************************ Expected:\n ************************************************\n");
		fprintf(stderr, "%s", expected_output);
		fprintf(stderr, "\n********************************** Got:\n *****************************************************\n");
		fprintf(stderr, "%s\n", test_output);
		return 1;
	}
}

//returns true if expected output is somewhere in the output
int match_part (char *expected_output)
{
	check_num++;
	if (strstr(test_output, expected_output) != NULL) {
		fprintf(stderr, "%s: check %d passed\n", global_test_name, check_num);
		return 0;
	} else {
		fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
        fprintf(stderr, "************************************ Expected:\n ************************************************\n");
		fprintf(stderr, "%s", expected_output);
        fprintf(stderr, "************************************ Got:\n *****************************************************\n");
		fprintf(stderr, "%s\n", test_output);
		return 1;
	}
}
