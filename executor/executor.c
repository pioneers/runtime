#include "executor.h"

void sigintHandler(int sig_num) {
	printf("===Terminating using Ctrl+C===\n");
    fflush(stdout);
    executor_stop();
	exit(0);
}

void executor_init() {
    shm_init(EXECUTOR);
}

void executor_stop() {
    shm_stop(EXECUTOR);
}


void spawn_thread(void* py_function) {

}

void start_mode(mode mode) {
    if (mode == AUTO) {

    }
    else if (mode == TELEOP) {

    }
    else if (mode == ESTOP) {
        executor_stop();
    }
}

int main(int argc, char* argv[]) {
    signal(SIGINT, sigintHandler);

    return 0;
}