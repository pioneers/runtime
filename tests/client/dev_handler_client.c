#include "dev_handler_client.h"

// Timeout time in ms for a socket read()
#define TIMEOUT 2000

// The name of a socket's file to be preceded by the home directory and succeeded by an integer
#define SOCKET_PREFIX "ttyACM"
const char* home_dir;  // holds home directory path

// The maximum number of devices that can be connected
#define MAX_DEVICES 32

// Process id of the dev_handler fork
pid_t dev_handler_pid;

// Struct grouping a virtual device's process id, socket fd, and name
typedef struct {
    pid_t pid;         // The process id of the virtual device
    int fd;            // The socket's file descriptor
    char* dev_name;    // The name of the virtual device's name
    uint64_t dev_uid;  // The device uid
} device_socket_t;

// ****************************** GLOBAL VARS ******************************* //
/* Array of connected devices and their sockets
 * used_sockets[i] == NULL if unused
 */
device_socket_t** used_sockets;
// A hack to initialize. https://stackoverflow.com/a/6991475
__attribute__((constructor)) void used_sockets_init() {
    used_sockets = malloc(MAX_DEVICES * sizeof(device_socket_t*));
    if (used_sockets == NULL) {
        printf("used_sockets_init: Failed to malloc\n");
        exit(1);
    }
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

    //    printf("successfully (?) created socket: server_fd = %d\n", server_fd);  // *******************************************************************************************
    //    fflush(stdout);

    // Build socket name
    char socket_name[64];
    sprintf(socket_name, "%s/%s%d", home_dir, SOCKET_PREFIX, socket_num);
    printf("socket_name = %s\n", socket_name);
    fflush(stdout);

    // Build socket address
    struct sockaddr_un dev_socket_addr;
    dev_socket_addr.sun_family = AF_UNIX;
    strcpy(dev_socket_addr.sun_path, socket_name);

    // Bind socket address to socket
    if (bind(server_fd, (struct sockaddr*) &dev_socket_addr, sizeof(dev_socket_addr)) < 0) {
        printf("connect_socket: Couldn't bind socket -- %s\n", strerror(errno));
        remove(socket_name);
        return -1;
    }
	
	printf("listening...\n");
	fflush(stdout);

    // Listen for dev_handler to connect to the virtual device socket
    listen(server_fd, 1);

    // Accept the connection
    int connection_fd;
	printf("listening before accept ...\n");
    if ((connection_fd = accept(server_fd, NULL, NULL)) < 0) {
        printf("connect_socket: Couldn't accept socket connection\n");
        return -1;
    }
    close(server_fd);

    //    printf("successful connection from dev_handler to device\n");
    //    fflush(stdout);

    // Indicate in global variable that socket is now used
    used_sockets[socket_num] = malloc(sizeof(device_socket_t));
    if (used_sockets[socket_num] == NULL) {
        printf("connect_socket: Failed to malloc\n");
        exit(1);
    }
    used_sockets[socket_num]->fd = connection_fd;

    // Set read() to timeout for up to TIMEOUT milliseconds
    struct timeval tv;
    tv.tv_sec = TIMEOUT / 1000;
    tv.tv_usec = 0;
    setsockopt(connection_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof(tv));

    printf("socket_num = %d\n", socket_num);  // *******************************************************************************************
    fflush(stdout);

    return socket_num;
}

// ******************************** Public ********************************* //

void start_dev_handler() {
    // Check to see if creation of child is successful
    if ((dev_handler_pid = fork()) < 0) {
        printf("fork: %s\n", strerror(errno));
    } else if (dev_handler_pid == 0) {  // child created!
        // redirect to dev handler folder
        if (chdir("../dev_handler") == -1) {
            printf("chdir: %s\n", strerror(errno));
        }
        // execute the device handler process
        if (execlp("./../bin/dev_handler", "dev_handler", (char*) 0) < 0) {
            printf("start_dev_handler execlp: %s\n", strerror(errno));
        }
    } else {  // in parent
        home_dir = getenv("HOME");
    }
}

void stop_dev_handler() {
    // send signal to dev_handler and wait for termination
    if (kill(dev_handler_pid, SIGINT) < 0) {
        printf("kill dev_handler:  %s\n", strerror(errno));
    }
    if (waitpid(dev_handler_pid, NULL, 0) < 0) {
        printf("waitpid dev_haandler: %s\n", strerror(errno));
    }
}

int connect_virtual_device(char* dev_name, uint64_t uid) {
    // Connect a socket
    int socket_num = connect_socket();
    if (socket_num == -1) {
        return -1;
    }

    // Take note of the type of device connected
    used_sockets[socket_num]->dev_name = malloc(strlen(dev_name));
    if (used_sockets[socket_num] == NULL) {
        printf("connect_virtual_device: Failed to malloc\n");
        exit(1);
    }
    strcpy(used_sockets[socket_num]->dev_name, dev_name);
    used_sockets[socket_num]->dev_uid = uid;

    // Fork to make child process execute virtual device
    pid_t pid = fork();
    if (pid < 0) {
        printf("connect_device: Couldn't spawn child process %s\n", dev_name);
        return -1;
    } else if (pid == 0) {  // Child process
        // Cd into virtual_devices dir where the device exe is
        if (chdir("bin/virtual_devices") == -1) {
            printf("chdir: %s\n", strerror(errno));
        }

        // Become the virtual device by calling "./<dev_name> <fd> <uid>"
        char exe_name[32], fd_str[4], uid_str[20];
        sprintf(exe_name, "./%s", used_sockets[socket_num]->dev_name);
        sprintf(fd_str, "%d", used_sockets[socket_num]->fd);
        printf("fd_str = %s\n", fd_str);  // **********************************************************************
        fflush(stdout);
        sprintf(uid_str, "0x%016llX", uid);
        if (execlp(exe_name, used_sockets[socket_num]->dev_name, fd_str, uid_str, (char*) NULL) < 0) {
            printf("connect_device: execlp %s failed -- %s\n", exe_name, strerror(errno));
            return -1;
        }
    } else {  // Parent process
        // Take note of child pid so we can kill it in disconnect_device()
        used_sockets[socket_num]->pid = pid;
		
		// close duplicate connection file descriptor
		close(used_sockets[socket_num]->fd);
    }
    return socket_num;
}

int disconnect_virtual_device(int socket_num) {
    // Do nothing if socket is unused
    if ((socket_num < 0) || (socket_num >= MAX_DEVICES) || (used_sockets[socket_num] == NULL)) {
        return -1;
    }
    // Kill virtual device process
    kill(used_sockets[socket_num]->pid, SIGTERM);
    waitpid(used_sockets[socket_num]->pid, NULL, 0);  // Block until killed
                                                      // CLose file descriptor
    printf("DEV HANDLER CLIENT CLOSE ?????? heh? \n");
    fflush(stdout);
    close(used_sockets[socket_num]->fd);
    // Remove socket (dev handler should recognize disconnect after removal)
    char socket_name[32];
    sprintf(socket_name, "%s/%s%d", home_dir, SOCKET_PREFIX, socket_num);
    remove(socket_name);
    // Mark socket as unoccupied
    free(used_sockets[socket_num]->dev_name);
    free(used_sockets[socket_num]);
    used_sockets[socket_num] = NULL;
    return 0;
}

void disconnect_all_devices() {
    for (int i = 0; i < MAX_DEVICES; i++) {
        disconnect_virtual_device(i);
    }
}

void list_devices() {
    printf("\n");
    printf("SOCKET\tDEVICE NAME\t\tUID\n");
    fflush(stdout);
    for (int i = 0; i < MAX_DEVICES; i++) {
        // If socket is used, show information
        if (used_sockets[i] != NULL) {
            printf("  %d\t%s\t0x%016llX\n", i, used_sockets[i]->dev_name, used_sockets[i]->dev_uid);
            fflush(stdout);
        }
    }
    printf("\n");
    return;
}
