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

void display_help() {
    printf("This is the list of commands.\n");
    printf("All commands should be typed in all lower case.\n");
    printf("\tstop          stop dev handler process\n");
    printf("\tconnect       connect specified device\n");
    printf("\tdisconnect    disconnect device from specified port\n");
    printf("\tlist          list all currently connected devices and their ports\n");
}

// ********************************** MAIN PROCESS ****************************************** //

void sigint_handler(int signum) {
    stop_dev_handler();
    exit(0);
}

int main() {
    char nextcmd[MAX_CMD_LEN];
    int stop = 1;

    signal(SIGINT, sigint_handler);

    printf("Starting Dev Handler CLI...\n");
    fflush(stdout);
    printf("Please enter device handler command:\n");
    fflush(stdout);
    start_dev_handler();

    while (stop) {
        // Get the next command
        printf("> ");
        fgets(nextcmd, MAX_CMD_LEN, stdin);
        nextcmd[strlen(nextcmd) - 1] = '\0'; // Strip off \n character

        // Compare input string against the available commands
        if (strcmp(nextcmd, "stop") == 0) {
            stop = 0;
        } else if (strcmp(nextcmd, "connect") == 0) {
            int is_device = 0;
            // Keep asking until user gives valid device name
            while (!is_device) {
                printf("This is the list of devices by name.\n");
                for (int i = 0; i < NUMBER_OF_TEST_DEVICES; i++) {
                    printf("\t%s\n", devices[i]);
                }
                // Ask for which device to connect
                printf("Device name?\n");
                fflush(stdout);
                printf("> ");
                fgets(nextcmd, MAX_CMD_LEN, stdin);
                nextcmd[strlen(nextcmd) - 1] = '\0'; // Strip off \n character
                for (int i = 0; i < NUMBER_OF_TEST_DEVICES; i++) {
                    if (strcmp(nextcmd, devices[i]) == 0) {
                        connect_device(nextcmd);
                        printf("Device Connected!\n");
                        is_device = 1;
                        break;
                    }
                }
                if (!is_device) {
                    printf("Invalid Device! \n");
                }
            }
        } else if (strcmp(nextcmd, "disconnect") == 0) {
            printf("Port Number?\n");
            fflush(stdout);
            printf("> ");
            fgets(nextcmd, MAX_CMD_LEN, stdin);
            nextcmd[strlen(nextcmd) - 1] = '\0'; // Strip off \n character
            int port_num = atoi(nextcmd);
            disconnect_device(port_num);
        } else if (strcmp(nextcmd, "list") == 0) {
            list_devices();
        } else {
            display_help();
        }
    }

    printf("Exiting Dev Handler CLI...\n");

    stop_dev_handler();

    printf("Done!\n");

    return 0;
}
