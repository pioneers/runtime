#include "shm_client.h"

void start_shm() {
    pid_t shm_pid;

    // fork shm_start process
    if ((shm_pid = fork()) < 0) {
        printf("fork: %s\n", strerror(errno));
    } else if (shm_pid == 0) {  // child
        // cd to the shm_wrapper directory
        if (chdir("../shm_wrapper") == -1) {
            printf("chdir: %s\n", strerror(errno));
        }

        // exec the shm_start process
        if (execlp("./../bin/shm_start", "shm", NULL) < 0) {
            printf("execlp: %s\n", strerror(errno));
        }
    } else {  // parent
        // wait for shm_start process to terminate
        if (waitpid(shm_pid, NULL, 0) < 0) {
            printf("waitpid shm: %s\n", strerror(errno));
        }

        // init to the now-existing shared memory
        shm_init();
    }
}

void stop_shm() {
    pid_t shm_pid;

    // fork shm_stop process
    if ((shm_pid = fork()) < 0) {
        printf("fork: %s\n", strerror(errno));
    } else if (shm_pid == 0) {  // child
        // cd to the shm_wrapper directory
        if (chdir("../shm_wrapper") == -1) {
            printf("chdir: %s\n", strerror(errno));
        }

        // exec the shm_stop process
        if (execlp("./../bin/shm_stop", "shm", NULL) < 0) {
            printf("execlp: %s\n", strerror(errno));
        }
    } else {  // parent
        // wait for shm_stop process to terminate
        if (waitpid(shm_pid, NULL, 0) < 0) {
            printf("waitpid shm: %s\n", strerror(errno));
        }
    }
}
