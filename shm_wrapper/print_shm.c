#include <stdio.h>
#include "shm_wrapper.h"

int main(int argc, char* argv[]) {
    uint32_t devices = 0;
    if (argc == 1) {
        devices = -1;
    }
    else {
        for (int i = 1; i < argc; i++) {
            devices |= (1 << atoi(argv[i]));
        }
    }
    logger_init(TEST);
    shm_init(TEST);
    print_params(devices);
}