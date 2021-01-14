#include <unistd.h>

/**
 * UnresponsiveTestDevice, a virtual device that doesn't respond to dev handler
 * Might as well do nothing
 * Tests dev handler hotplugging timeout for not receiving an ACK
 * Arguments:
 *    int: file descriptor for the socket
 *    uint64_t: device uid
 */
int main(int argc, char* argv[]) {
    // Do nothing. He's in for a long nap.
    while (1) {
        sleep(60);
    }
    return 0;
}
