#include "dev_handler_client.h"

// Timeout time in ms for a socket read()
#define TIMEOUT 2000

// The name of a socket's file to be succeeded by an integer
#define SOCKET_PREFIX "/tmp/ttyACM"

// The maximum number of devices that can be connected
#define MAX_DEVICES 32

// Struct grouping a virtual device's process id, socket fd, and name
typedef struct {
    pid_t pid;          // The process id of the virtual device
    int fd;             // The socket's file descriptor
    char *dev_name; // The name of the virtual device's name
} device_socket_t;

// ****************************** GLOBAL VARS ******************************* //
/* Array of connected devices and their sockets
 * used_sockets[i] == NULL if unused
 */
device_socket_t **used_sockets;
// A hack to initialize. https://stackoverflow.com/a/6991475
__attribute__((constructor))
void used_sockets_init() {
    used_sockets = malloc(MAX_DEVICES * sizeof(device_socket_t *));
    for (int i = 0; i < MAX_DEVICES; i++) {
        used_sockets[i] = NULL;
    }
}

// ******************************** Private ********************************* //

/**
 * Returns the socket number after connecting to an available socket
 * Returns:
 *    The socket number, or
 *    -1 if error
 */
static int connect_socket() {
    // Get an unoccupied socket and get a fd
    int socket_num = -1;
    for (int i = 0; i < 32; i++) {
        if (used_sockets[i] == NULL) {
            socket_num = i;
            break;
        }
    }
    // Error if no sockets available
    if (socket_num == -1) {
        return -1;
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
    used_sockets[socket_num] = malloc(sizeof(device_socket_t));
    used_sockets[socket_num]->fd = connection_fd;

    // Set read() to timeout for up to TIMEOUT milliseconds
    struct timeval tv;
    tv.tv_sec = TIMEOUT / 1000;
    tv.tv_usec = 0;
    setsockopt(connection_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    return socket_num;
}

// ******************************** Public ********************************* //

void start_dev_handler() {
    return;
}

void stop_dev_handler() {
    return;
}

// TODO: Require year and UID
int connect_device(char* dev_name) {
    // Connect a socket
    int socket_num = connect_socket();
    if (socket_num == -1) {
        return -1;
    }
    // printf("Successfuly connected to socket %s%d\n", SOCKET_PREFIX, socket_num);

    // Take note of the type of device connected
    used_sockets[socket_num]->dev_name = malloc(strlen(dev_name));
    strcpy(used_sockets[socket_num]->dev_name, dev_name);

    // Fork to make child process execute virtual device
    pid_t pid = fork();
    if (pid < 0) {
        printf("connect_device: Couldn't spawn child process %s\n", dev_name);
        return -1;
    } else if (pid == 0) { // Child process
        // Cd into virtual_devices dir where the device exe is
        if (chdir("client/virtual_devices") == -1) {
            printf("chdir: %s\n", strerror(errno));
        }
        // Become the virtual device by calling "./<dev_name> <fd>"
        char exe_name[32];
        sprintf(exe_name, "./%s", used_sockets[socket_num]->dev_name);
        char fd_str[2]; // CLI arg to virtual device
        sprintf(fd_str, "%d", used_sockets[socket_num]->fd);
        if (execlp(exe_name, used_sockets[socket_num]->dev_name, fd_str, (char *) NULL) < 0) {
            printf("connect_device: execlp %s failed -- %s\n", exe_name, strerror(errno));
        }
    } else { // Parent process
        // Take note of child pid so we can kill it in disconnect_device()
        used_sockets[socket_num]->pid = pid;
    }
    return 0;
}

int disconnect_device(int socket_num) {
    // Do nothing if socket is unused
    if ((socket_num < 0) || (socket_num >= MAX_DEVICES) || (used_sockets[socket_num] == NULL)) {
        return -1;
    }
    // Kill virtual device process
    kill(used_sockets[socket_num]->pid, SIGTERM);
    waitpid(used_sockets[socket_num]->pid, NULL, 0); // Block until killed
    // CLose file descriptor
    close(used_sockets[socket_num]->fd);
    // Remove socket (dev handler should recognize disconnect after removal)
    char socket_name[14];
    sprintf(socket_name, "%s%d", SOCKET_PREFIX, socket_num);
    remove(socket_name);
    // Mark socket as unoccupied
    free(used_sockets[socket_num]->dev_name);
    free(used_sockets[socket_num]);
    used_sockets[socket_num] = NULL;
    return 0;
}

void disconnect_all_devices() {
    for (int i = 0; i < MAX_DEVICES; i++) {
        disconnect_device(i);
    }
}

// TODO: List UIDs as well
void list_devices() {
    printf("\n");
    printf("SOCKET\tDEVICE\n");
    fflush(stdout);
    for (int i = 0; i < MAX_DEVICES; i++) {
        // If socket is used, show information
        if (used_sockets[i] != NULL) {
            printf("  %d\t%s\n", i, used_sockets[i]->dev_name);
            fflush(stdout);
        }
    }
    printf("\n");
    return;
}
