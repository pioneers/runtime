#include "executor_client.h"

pid_t executor_pid;

// for adding student code directory to PYTHONPATH
char* path_to_test_student_code = "../tests/student_code";
char* PYTHONPATH = "PYTHONPATH";

void start_executor(char* student_code) {
    // fork executor process
    if ((executor_pid = fork()) < 0) {
        printf("fork: %s\n", strerror(errno));
    } else if (executor_pid == 0) {  // child
        char* python_path = NULL;
        char* new_python_path = malloc(strlen(path_to_test_student_code) + 1);
        if (new_python_path == NULL) {
            printf("start_executor: Failed to malloc\n");
            exit(1);
        }
        int len;

        // cd to the executor directory
        if (chdir("../executor") == -1) {
            printf("chdir: %s\n", strerror(errno));
        }

        // add the directory with all the student code to PYTHONPATH if it's not already there
        if ((python_path = getenv(PYTHONPATH)) == NULL) {  // if PYTHONPATH is not defined
            strcpy(new_python_path, path_to_test_student_code);
        } else {  // if PYTHONPATH is defined
            new_python_path = realloc(new_python_path, strlen(python_path) + 1 + strlen(path_to_test_student_code) + 1);
            if (new_python_path == NULL) {
                fprintf(stderr, "start_executor: Failed to realloc\n");
                exit(1);
            }
            new_python_path[0] = '\0';
            strcat(new_python_path, python_path);
            strcat(new_python_path, ":");
            strcat(new_python_path, path_to_test_student_code);
        }
        if (setenv(PYTHONPATH, new_python_path, 1) != 0) {
            fprintf(stderr, "start_executor: failed to set PYTHONPATH: %s\n", strerror(errno));
        }
        free(new_python_path);  // setenv copies the strings so we need to free the allocated pointer

        // get rid of the newline at the end of the file name, if it's there
        len = strlen(student_code);
        if (student_code[len - 1] == '\n') {
            student_code[len - 1] = '\0';
        }
        // exec the actual executor process
        if (execlp("./../bin/executor", "executor", student_code, NULL) < 0) {
            printf("execlp: %s\n", strerror(errno));
        }
    }
}

void stop_executor() {
    // send signal to executor and wait for termination
    if (kill(executor_pid, SIGINT) < 0) {
        printf("kill executor:  %s\n", strerror(errno));
    }
    if (waitpid(executor_pid, NULL, 0) < 0) {
        printf("waitpid executor: %s\n", strerror(errno));
    }
}
