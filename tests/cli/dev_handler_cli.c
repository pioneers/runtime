#include "../client/dev_handler_client.h"

#define MAX_CMD_LEN 64

#define NUMBER_OF_TEST_DEVICES 5
#define MAX_STRING_SIZE 64

char devices[NUMBER_OF_TEST_DEVICES][MAX_STRING_SIZE] =
{ "GeneralTestDevice",
"SimpleTestDevice",
"UnresponsiveTestDevice",
"ForeignTestDevice",
"UnstableTestDevice"
};

char nextcmd[MAX_CMD_LEN];

void display_help() {
    printf("This is the list of commands.\n");
    printf("All commands should be typed in all lower case.\n");
    printf("\thelp          show this menu of commands\n");
    printf("\tstop          stop dev handler process\n");
    printf("\tconnect       connect specified device\n");
    printf("\tdisconnect    disconnect device from specified socket number\n");
    printf("\tlist          list all currently connected devices and their ports\n");
}
// ********************************** COMMAND-SPECIFIC FUNCTIONS  ****************************** //

void clean_up() {
    printf("Exiting Dev Handler CLI...\n");
    disconnect_all_devices();
    stop_dev_handler();
    printf("Done!\n");
    exit(0);
}

void remove_newline(char *nextcmd) {
    nextcmd[strlen(nextcmd) - 1] = '\0';
}

void prompt_device_connect() {
    while(1) {
        printf("This is the list of devices by name. Type in number on left to use device\n");
        for (int i = 0; i < NUMBER_OF_TEST_DEVICES; i++) {
            printf("\t%d    %s\n", i, devices[i]);
        }
        printf("\t5    Cancel\n");
        printf("Device?\n");
        fflush(stdout);
        printf("> ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        char *input;
        long int dev_number; 
        dev_number = strtol(nextcmd, &input, 0);
        if(dev_number >= 0 && dev_number < 5 && strlen(input) == 1 && strlen(nextcmd) > 1) {
            printf("UID for device?\n");
            fflush(stdout);
            printf("> ");
            fgets(nextcmd, MAX_CMD_LEN, stdin);
            uint64_t uid;
            remove_newline(nextcmd);
            uid = strtoull(nextcmd, NULL, 0);
            printf("Connecting %s with UID of value %llu\n", devices[dev_number], uid);
            if(connect_device(devices[dev_number], uid) == 0) {
                fflush(stdout);
                printf("Device connected!\n");
                break;
            } else {
                printf("Device connection failed!\n");
                break;
                }
        } else if (dev_number == 5) {
            printf("Device connecting cancelled \n");
            break;
        } else {
            printf("Invalid Device! \n");
        }
    }
}

void prompt_device_disconnect() {
    while(1) {
        list_devices();
        printf("Socket Number?\n");
        fflush(stdout);
        printf("> ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        remove_newline(nextcmd);
        char *input;
        int port_num = strtol(nextcmd, &input, 0);
        if(strlen(input) == 0 && strlen(input) == 0 && strlen(nextcmd) > 0) {
            printf("Disconnecting port %d\n", port_num);
            if(disconnect_device(port_num) == 0) {
                printf("Device disconnected!\n");
                break;
            } else {
                printf("Device disconnect unsuccesful!\n");
                break;
            }
        } else if(strcmp(input, "cancel") == 0) {
            printf("Cancelling disconnect!\n");
            break;
        } else {
            printf("Invalid input!\n");
        }
    }
}

// ********************************** MAIN PROCESS ****************************************** //

int main() {
    signal(SIGINT, clean_up);

    int stop = 1;

    printf("Starting Dev Handler CLI in the cloudddd...\n");
    fflush(stdout);
    display_help();
    printf("Please enter device handler command:\n ");
    fflush(stdout);
    start_dev_handler();

    while (stop) {
        // Get the next command
        printf("> ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        remove_newline(nextcmd); // Strip off \n character
        // Compare input string against the available commands
        if (strcmp(nextcmd, "stop") == 0) {
            stop = 0;
        } else if (strcmp(nextcmd, "connect") == 0) {
            prompt_device_connect();
        } else if (strcmp(nextcmd, "disconnect") == 0) {
            prompt_device_disconnect();
        } else if (strcmp(nextcmd, "list") == 0) {
            list_devices();
        } else if (strcmp(nextcmd, "help") == 0) {
            display_help();
        }
    }

    clean_up();
    return 0;
}
