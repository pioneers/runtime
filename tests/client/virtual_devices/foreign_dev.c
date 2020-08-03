#include <stdlib.h> // for srand() and rand()
#include <time.h>   // for time() as a seed
#include <stdint.h> // for uint8_t
#include <unistd.h> // for write()

/**
 * ForeignTestDevice, a virtual device that sends nonsense to dev handler
 * Tests dev handler indentification of a non-lowcar device
 * Arguments:
 *    int: file descriptor for the socket
 *    uint64_t: device uid
 */
int main(int argc, char *argv[]) {
    // https://www.tutorialspoint.com/c_standard_library/c_function_srand.htm
    // Initialize random seed using the current time
    srand(time(NULL));

    // Send random bytes to the socket
    int fd = atoi(argv[1]);
    uint8_t byte_to_send = 0;
    // Write the 0x00 byte first to see how dev handler responds to a bad message
    write(fd, &byte_to_send, 1);
    while (1) {
        byte_to_send = rand();
        write(fd, &byte_to_send, 1);
    }
}
