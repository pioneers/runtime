#include "executor_client.h"

pid_t executor_pid;

void start_executor (char *student_code)
{
	//fork shm process
	if ((executor_pid = fork()) < 0) {
		printf("fork: %s\n", strerror(errno));
	} else if (executor_pid == 0) { //child
		//cd to the executor directory
		if (chdir("../../executor") == -1) {
			printf("chdir: %s\n", strerror(errno));
		}
		
		//exec the actual executor process
		if (execlp("../../executor/executor", "executor", student_code, (char *) 0) < 0) {
			printf("execlp: %s\n", strerror(errno));
		}
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