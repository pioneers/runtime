#include "executor_client.h"

char *path_to_test_student_code = "../tests/student_code";
char *python_path_header = "PYTHONPATH=";
pid_t executor_pid;

void start_executor (char *student_code)
{
	//fork executor process
	if ((executor_pid = fork()) < 0) {
		printf("fork: %s\n", strerror(errno));
	} else if (executor_pid == 0) { //child
		char *python_path, *new_python_path;
		int len;
		//cd to the executor directory
		if (chdir("../../executor") == -1) {
			printf("chdir: %s\n", strerror(errno));
		}
		//add the directory with all the student code to PYTHONPATH
		if ((python_path = getenv("PYTHONPATH")) == NULL) {
			new_python_path = malloc(strlen(python_path_header) + strlen(path_to_test_student_code) + 1);
			new_python_path[0] = '\0';
			strcat(new_python_path, python_path_header);
			strcat(new_python_path, path_to_test_student_code);
		} else {
			new_python_path = malloc(strlen(python_path) + strlen(path_to_test_student_code) + strlen(python_path_header) + 1);
			new_python_path[0] = '\0';
			strcat(new_python_path, python_path_header);
			strcat(new_python_path, python_path);
			strcat(new_python_path, path_to_test_student_code);
		}
		if (putenv(new_python_path) != 0) {
			printf("putenv: %s\n", strerror(errno));
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

		//free pointers
		free(new_python_path);
	} else { //parent
		;
	}
}

void stop_executor ()
{
	//send signal to executor and wait for termination
	if (kill(executor_pid, SIGINT) < 0) {
		printf("kill: %s\n", strerror(errno));
	}
	if (waitpid(executor_pid, NULL, 0) < 0) {
		printf("waitpid: %s\n", strerror(errno));
	}
}
