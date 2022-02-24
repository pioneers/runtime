#include "../client/dev_handler_client.h"

#define MAX_CMD_LEN 64  // maximum length of a CLI command

#define NUMBER_OF_TEST_DEVICES 7  // number of test devices to connect
#define MAX_STRING_SIZE 64        // maximum length of a device name

char devices[NUMBER_OF_TEST_DEVICES][MAX_STRING_SIZE] = {
    "SoundDevice",
    "GeneralTestDevice",
    "SimpleTestDevice",
    "UnresponsiveTestDevice",
    "ForeignTestDevice",
    "UnstableTestDevice",
    "OtherTestDevice"};

char nextcmd[MAX_CMD_LEN];

void display_help() {
    printf("This is the list of commands. [TEST FOR PRINT REMOVE AFTER]\n");
    printf("All commands should be typed in all lower case.\n");
    printf("\thelp          show this menu of commands\n");
    printf("\texit          exit the device handler CLI\n");
    printf("\tconnect       connect specified device\n");
    printf("\tdisconnect    disconnect device from specified socket number\n");
    printf("\tlist          list all currently connected devices and their ports\n");
}

// True iff this CLI should attach to an already existing instance of dev handler rather than spawning a new one.
bool attach = 0;

// ********************************** COMMAND-SPECIFIC FUNCTIONS  ****************************** //

void clean_up() {
    printf("Exiting Dev Handler CLI...\n");
    disconnect_all_devices();
    if (!attach) {
        stop_dev_handler();
    }
    printf("Done!\n");
    exit(0);
}

// removes trailing newline from commands entered in by user
void remove_newline(char* nextcmd) {
    nextcmd[strlen(nextcmd) - 1] = '\0';
}

void prompt_device_connect() {
    while (true) {
        // prints out list of available test devices
        printf("This is the list of devices by name. Type in number on left to use device\n");
        for (int i = 0; i < NUMBER_OF_TEST_DEVICES; i++) {
            printf("\t%d    %s\n", i, devices[i]);
        }
        printf("\t%d    Cancel\n", NUMBER_OF_TEST_DEVICES);
        printf("Device?\n");
        fflush(stdout);
        printf("> ");

        // get next command entered by user and process it
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        char* input;
        long int dev_number = strtol(nextcmd, &input, 0);
        if (dev_number >= 0 && dev_number < NUMBER_OF_TEST_DEVICES && strlen(input) == 1 && strlen(nextcmd) > 1) {
            // get the UID of the device
            printf("UID for device?\n");
            fflush(stdout);
            printf("> ");
            fgets(nextcmd, MAX_CMD_LEN, stdin);
            uint64_t uid;
            remove_newline(nextcmd);
            uid = strtoull(nextcmd, NULL, 0);

            // connect the virtual device
            printf("Connecting %s with UID of value 0x%016llX\n", devices[dev_number], uid);
            if (connect_virtual_device(devices[dev_number], uid) == -1) {
                printf("Device connection failed!\n");
            } else {
                printf("Device connected!\n");
            }
            fflush(stdout);
            break;
        } else if (dev_number == NUMBER_OF_TEST_DEVICES) {  // if we connected too many devices
            printf("Device connecting cancelled \n");
            break;
        } else {
            printf("Invalid Device! \n");
        }
    }
}

void prompt_device_disconnect() {
    while (true) {
        // list the connected virtual devices
        list_devices();

        // ask for the device to disconnect
        printf("Socket Number?\n");
        fflush(stdout);
        printf("> ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        remove_newline(nextcmd);
        char* input;
        int port_num = strtol(nextcmd, &input, 0);

        // perform some validation on the input, then disconnect the virtual device
        if (strlen(input) == 0 && strlen(input) == 0 && strlen(nextcmd) > 0) {
            printf("Disconnecting port %d\n", port_num);
            if (disconnect_virtual_device(port_num) == 0) {
                printf("Device disconnected!\n");
                sleep(1);  // Let dev handler output logs
                break;
            } else {
                printf("Device disconnect unsuccesful!\n");
                break;
            }
        } else if (strcmp(input, "cancel") == 0) {  // break immediately if cancelling
            printf("Cancelling disconnect!\n");
            break;
        } else {
            printf("Invalid input!\n");
        }
    }
}

// ********************************** MAIN PROCESS ****************************************** //

int main(int argc, char** argv) {
    // setup
    signal(SIGINT, clean_up);
    bool stop = 1;

    // If the argument "attach" is specified, then set the global variable
    if (argc == 2 && strcmp(argv[1], "attach") == 0) {
        attach = true;
    }

    // Start dev handler if we aren't attaching to existing dev handler
    if (!attach) {
        start_dev_handler();
        sleep(1);  // Allow dev handler to initialize
    }

    // start dev handler
    printf("Starting Dev Handler CLI in the cloudddd...\n");
    fflush(stdout);
    display_help();
    printf("Please enter device handler command:\n");
    fflush(stdout);

    // main loop
    while (stop) {
        // Get the next command
        sleep(1);  // Guarantee that the "> " prompt appears after dev handler logs
        printf("> ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        remove_newline(nextcmd);  // Strip off \n character
        // Compare input string against the available commands
        if (strcmp(nextcmd, "exit") == 0) {
            stop = 0;
        } else if (strcmp(nextcmd, "connect") == 0) {
            prompt_device_connect();
        } else if (strcmp(nextcmd, "disconnect") == 0) {
            prompt_device_disconnect();
        } else if (strcmp(nextcmd, "list") == 0) {
            list_devices();
        } else {
            display_help();
        }
    }

    clean_up();
    return 0;
}
