#include "test.h"

#define TEMP_FILE "temp.txt"

char *test_output;              //holds the contents of temp file
int temp_fd;                    //file descriptor of open temp file
int check_num = 0;              //increments to report status each time a check is performed
char *global_test_name = NULL;  //name of test

//removes the temporary file on exit
static void on_exit ()
{
	close(temp_fd);
	remove(TEMP_FILE);
	free(global_test_name);
	free(test_output);
}

//routes stdout of test process to a temporary file to collect output from all the clients
void start_test (char *test_name)
{
	//save the test name
	global_test_name = malloc(strlen(test_name));
	strcpy(global_test_name, test_name);
	
	//open the temp file
	if ((temp_fd = open(TEMP_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
		printf("open: cannot create temp file: %s\n", strerror(errno));
	}
	//route standard output to this file
	if (dup2(STDOUT_FILENO, temp_fd) != temp_fd) {
		fprintf(stderr, "dup2 stdout to temp_fd: %s\n", strerror(errno));
	}
	close(STDOUT_FILENO);
	atexit(on_exit);
}

//this is called when the test has shut down all runtime processes and is ready to compare output
//reads in the entire contents of the temporary file into a string
void end_test ()
{
	size_t curr_size = 1024;
	size_t num_total_bytes_read = 0;
	FILE *temp_fp = fdopen(temp_fd, "r");
	char *curr_ptr = test_output = malloc(curr_size);
	
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
}

//returns true if exepcted output matches output exactly
int match_all (char *expected_output)
{
	check_num++;
	if (strcmp(expected_output, test_output) == 0) {
		fprintf(stderr, "%s: check %d passed\n", global_test_name, check_num);
		return 0;
	} else {
		fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
		fprintf(stderr, "Expected:\n");
		fprintf(stderr, "%s", expected_output);
		fprintf(stderr, "\nGot:\n");
		fprintf(stderr, "%s\n", test_output);
		return 1;
	}
}

//returns true if expected output is somewhere in the output
int match_part (char *expected_output)
{
	check_num++;
	if (strstr(expected_output, test_output) != NULL) {
		fprintf(stderr, "%s: check %d passed\n", global_test_name, check_num);
		return 0;
	} else {
		fprintf(stderr, "%s: check %d failed\n", global_test_name, check_num);
		fprintf(stderr, "Expected:\n");
		fprintf(stderr, "%s", expected_output);
		fprintf(stderr, "\nGot:\n");
		fprintf(stderr, "%s\n", test_output);
		return 1;
	}
}
