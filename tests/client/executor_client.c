#include "executor_client.h"

pid_t executor_pid;

//for adding student code directory to PYTHONPATH
char *path_to_test_student_code = "../tests/student_code";
char *python_path_header = "PYTHONPATH=";

void start_executor (char *student_code)
{
	//if someone presses Ctrl-C (SIGINT), stop executor process
	signal(SIGINT, stop_executor);

	//fork executor process
	if ((executor_pid = fork()) < 0) {
		printf("fork: %s\n", strerror(errno));
	} else if (executor_pid == 0) { //child
		char *python_path = NULL, *new_python_path = NULL;
		int len;

		//cd to the executor directory
		if (chdir("../executor") == -1) {
			printf("chdir: %s\n", strerror(errno));
		}

		//add the directory with all the student code to PYTHONPATH if it's not already there
		if ((python_path = getenv("PYTHONPATH")) == NULL) { //if PYTHONPATH is not defined
			new_python_path = malloc(strlen(python_path_header) + strlen(path_to_test_student_code) + 1);
			new_python_path[0] = '\0';
			strcat(new_python_path, python_path_header);
			strcat(new_python_path, path_to_test_student_code);
		} else if (strstr(python_path, "tests/student_code") == NULL) { //if PYTHONPATH is defined but the path we want isn't there
			new_python_path = malloc(strlen(python_path) + strlen(path_to_test_student_code) + strlen(python_path_header) + 1);
			new_python_path[0] = '\0';
			strcat(new_python_path, python_path_header);
			strcat(new_python_path, python_path);
			strcat(new_python_path, path_to_test_student_code);
		}
		if (new_python_path != NULL) { //if one of the two above cases occurred, putenv the new PYTHONPATH
			if (putenv(new_python_path) != 0) {
				printf("putenv: %s\n", strerror(errno));
			}
		}

		//get rid of the newline at the end of student_code, if it's there
		len = strlen(student_code);
		if (student_code[len - 1] == '\n') {
			student_code[len - 1] = '\0';
		}

		//exec the actual executor process
		if (execlp("./executor", "executor", student_code, (char *) 0) < 0) {
			printf("execlp: %s\n", strerror(errno));
		}
	}
}

void stop_executor ()
{
	//send signal to executor and wait for termination
	if (kill(executor_pid, SIGINT) < 0) {
		printf("kill executor:  %s\n", strerror(errno));
	}
	if (waitpid(executor_pid, NULL, 0) < 0) {
		printf("waitpid executor: %s\n", strerror(errno));
	}
}
