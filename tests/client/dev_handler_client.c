#include "dev_handler_client.h"

#define TIMEOUT 2000

#define SOCKET_PREFIX "/tmp/ttyACM"

#define MAX_DEVICES 32

// ****************************** GLOBAL VARS ******************************* //
// Array of file descriptors. used_sockets[i] == file descriptor for "/tmp/ttyACM[i]"
// If used_sockets[i] == -1, it is unused
int used_sockets[MAX_DEVICES];

// ******************************** Private ********************************* //
/**
 * Cleans up sockets
 */
void stop() {
    char socket_name[14];
    for (int i = 0; i < MAX_DEVICES; i++) {
        sprintf(socket_name, "%s%d", SOCKET_PREFIX, i);
        remove(socket_name);
    }
    exit(0);
}

// Utility
static void print_bytes(uint8_t *data, size_t len) {
    printf("0x");
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

/**
* Returns a file descriptor after connecting to a socket
* Returns:
*    A valid file descriptor, or
*    -1 if error
*/
static int connect_socket() {
    // Get an unoccupied socket and get a fd
    int socket_num;
    for (int i = 0; i < 32; i++) {
        if (used_sockets[i] == -1) {
            socket_num = i;
            break;
        }
    }
    // Make UNIX socket with raw byte streams
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        printf("connect_socket: Couldn't create socket\n");
        return -1;
    }

    // Build socket name
    char socket_name[14];
    sprintf(socket_name, "%s%d", SOCKET_PREFIX, socket_num);

    // Build socket address
    struct sockaddr_un dev_socket_addr;
    dev_socket_addr.sun_family = AF_UNIX;
    strcpy(dev_socket_addr.sun_path, socket_name);

    // Bind socket address to socket
    if (bind(server_fd, (struct sockaddr *)&dev_socket_addr, sizeof(dev_socket_addr)) < 0) {
        printf("connect_socket: Couldn't bind socket -- %s\n", strerror(errno));
        return -1;
    }

    // Listen for dev_handler to connect to the virtual device socket
    listen(server_fd, 1);

    // Accept the connection
    int connection_fd;
    if ((connection_fd = accept(server_fd, NULL, NULL)) < 0) {
        printf("connect_socket: Couldn't accept socket connection\n");
        return -1;
    }
    close(server_fd);

    // Indicate in global variable that socket is now used
    used_sockets[socket_num] = connection_fd;

    // Set read() to timeout for up to TIMEOUT milliseconds
    struct timeval tv;
    tv.tv_sec = TIMEOUT / 1000;
    tv.tv_usec = 0;
    setsockopt(connection_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    return connection_fd;
}
// ******************************** Public ********************************* //

void start_dev_handler();

void stop_dev_handler();

void connect_device(char *device_name) {
    int fd = connect_socket();
    // Spawn virtual device with arg fd
    pid_t pid = fork();
    if (pid == 0) { // If child process
        // Become the virtual device
        char exe_name[32];
        sprintf(exe_name, "./%s", device_name);
        char fd_str[2];
        sprintf(fd_str, "%d", fd);
        if (execlp(exe_name, fd_str, (char *) NULL) < 0) {
			printf("connect_device: execlp failed -- %s\n", strerror(errno));
		}
    }
}

void disconnect_device(int socket_num) {
    // CLose file descriptor
    close(used_sockets[socket_num]);
    // Remove socket
    char socket_name[14];
    sprintf(socket_name, "%s%d", SOCKET_PREFIX, socket_num);
    remove(socket_name);
    // Mark socket as unoccupied
    used_sockets[socket_num] = -1;
}

void list_devices();

// For testing purposes. Creates and binds a general_dev
int main() {
    // If SIGINT (Ctrl+C) is received, call stop() to clean up
    signal(SIGINT, stop);
    // Reset used_sockets
    for (int i = 0; i < MAX_DEVICES; i++) {
        used_sockets[i] = -1;
    }
    printf("Connecting general_dev\n");
    connect_device("general_dev");
}
