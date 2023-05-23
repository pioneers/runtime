/**
 * SimpleTestDevice, A virtual device that acts like a proper lowcar device
 * The same as GeneralTestDevice except with only 4 parameters
 */
#include "virtual_device_util.h"

// SimpleTestDevice params
enum {
    // Read-only
    INCREASING,
    DOUBLING,
    FLIP_FLOP,
    // Read and Write
    MY_INT
};

/**
 * Initialize the values for each param
 * Arguments:
 *    params: Array of params to be initialized
 */
void init_params(param_val_t params[]) {
    params[INCREASING].p_i = 0;
    params[DOUBLING].p_f = 1;
    params[FLIP_FLOP].p_b = 1;
    params[MY_INT].p_i = 0;
}

/**
 * Changes device's read-only params
 * Arguments:
 *    params: Array of param values to be modified
 */
void device_actions(param_val_t params[]) {
    params[INCREASING].p_i += 1;
    params[DOUBLING].p_f *= 2;
    params[FLIP_FLOP].p_b = 1 - params[FLIP_FLOP].p_b;
}

/* check first 1024 (usual size of FD_SESIZE) file handles */
int test_fds()
{
    int i;
    int fd_dup;
    char errst[64];
    for (i = 0; i < FD_SETSIZE; i++) {
        *errst = 0;
        fflush(stdout);
        fd_dup = dup(i);
        if (fd_dup == -1) {
            // EBADF  oldfd isnâ€™t an open file descriptor, or newfd is out of the allowed range for file descriptors.
            // EBUSY  (Linux only) This may be returned by dup2() during a race condition with open(2) and dup().
            // EINTR  The dup2() call was interrupted by a signal; see signal(7).
            // EMFILE The process already has the maximum number of file descriptors open and tried to open a new one.
        } else {
            close(fd_dup);
            printf("File descriptor %d exists!\n", i);
            fflush(stdout);
        }
    }
    return 0;
}

/**
 * A device that behaves like a lowcar device, connected to dev handler via a socket
 * Arguments:
 *    int: file descriptor for the socket
 *    uint64_t: device uid
 */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Incorrect number of arguments: %d out of %d\n", argc, 3);
        exit(1);
    }
    
    printf("SimpleTestDevice arguments: %s %s %s\n", argv[0], argv[1], argv[2]);

    int fd = atoi(argv[1]);
    uint64_t uid = strtoull(argv[2], NULL, 0);

    uint8_t dev_type = device_name_to_type("SimpleTestDevice");
    device_t* dev = get_device(dev_type);

    param_val_t params[dev->num_params];
    init_params(params);
    
    printf("SimpleTestDevice communication socket fd = %d\n", fd);
    fflush(stdout);
    
    // // print out all open file descriptors
//     // long max_fd = sysconf(_SC_OPEN_MAX); // get maximum number of open file descriptors
//     for (int i = 0; i < 32; i++) {
//         int ret;
//         if ((ret = fcntl(i, F_GETFD)) == -1) {
//             printf("file descriptor %d exists!\n", ret);
//             fflush(stdout);
//         }
//     }
    
    test_fds();

    lowcar_protocol(fd, dev_type, dev_type, uid, params, &device_actions, 1000);
    return 0;
}
