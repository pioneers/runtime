#include "dev_handler_client.h"

// Timeout time in ms for a socket read()
#define TIMEOUT 2000

// The name of a socket's file to be succeeded by an integer
#define SOCKET_PREFIX "/tmp/ttyACM"

// The maximum number of devices that can be connected
#define MAX_DEVICES 32

// Struct grouping a virtual device's socket number, socket fd, and name
typedef struct {
    int fd;             // The socket's file descriptor
    char *dev_exe_name; // The name of the virtual device's executable file
} device_socket_t;

// ****************************** GLOBAL VARS ******************************* //
/* Array of connected devices and their sockets
 * used_sockets[i] == NULL if unused
 */
device_socket_t used_sockets[MAX_DEVICES];
// A hack to initialize. https://stackoverflow.com/a/6991475
__attribute__((constructor))
void used_sockets_init() {
    for (int i = 0; i < MAX_DEVICES; i++) {
        used_sockets[i].fd = -1;
        used_sockets[i].dev_exe_name = "";
    }
}

// ******************************** Private ********************************* //

/**
 * Returns a file descriptor after connecting to a socket
 * Returns:
 *    A valid file descriptor, or
 *    -1 if error
 */
static int connect_socket(char *dev_exe_name) {
    // Get an unoccupied socket and get a fd
    int socket_num;
    for (int i = 0; i < 32; i++) {
        if (used_sockets[i].fd == -1) {
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
    used_sockets[socket_num].fd = connection_fd;
    used_sockets[socket_num].dev_exe_name = dev_exe_name;

    // Set read() to timeout for up to TIMEOUT milliseconds
    struct timeval tv;
    tv.tv_sec = TIMEOUT / 1000;
    tv.tv_usec = 0;
    setsockopt(connection_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    return connection_fd;
}

// ******************************** Public ********************************* //

void start_dev_handler() {
    return;
}

void stop_dev_handler() {
    return;
}

void connect_device(char* dev_exe_name) {
    int fd = connect_socket(dev_exe_name);
    // Spawn virtual device with arg fd
    pid_t pid = fork();
    if (pid == 0) { // If child process
        // Become the virtual device
        char exe_name[32];
        sprintf(exe_name, "./%s", dev_exe_name);
        char fd_str[2];
        sprintf(fd_str, "%d", fd);
        if (execlp(exe_name, fd_str, (char *) NULL) < 0) {
            printf("connect_device: execlp failed -- %s\n", strerror(errno));
        }
    }
}

void disconnect_device(int socket_num) {
    // CLose file descriptor
    close(used_sockets[socket_num].fd);
    // Remove socket
    char socket_name[14];
    sprintf(socket_name, "%s%d", SOCKET_PREFIX, socket_num);
    remove(socket_name);
    // Mark socket as unoccupied
    used_sockets[socket_num].fd = -1;
    used_sockets[socket_num].dev_exe_name = "";
}

void list_devices() {
    printf("Socket\t\t\tDevice\n");
    for (int i = 0; i < MAX_DEVICES; i++) {
        // If socket is used, show information
        if (used_sockets[i].fd != -1) {
            printf("%s%d\t\t%s\n", SOCKET_PREFIX, i, used_sockets[i].dev_exe_name);
        }
    }
    return;
}
