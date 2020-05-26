#include "executor.h"

void sigintHandler(int sig_num) {
	printf("===Terminating using Ctrl+C===\n");
    fflush(stdout);
	exit(0);

	//shm_stop(<this process>)
}

int main(int argc, char* argv[]) {
    
    return 0;
}