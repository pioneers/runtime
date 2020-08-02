#include "../client/dev_handler_client.h"

#define MAX_CMD_LEN 64

#define NUMBER_OF_TEST_DEVICES 5
#define MAX_STRING_SIZE 64

char devices[NUMBER_OF_TEST_DEVICES][MAX_STRING_SIZE] =
{ "GeneralTestDevice\n",
"SimpleTestDevice\n",
"UnresponsiveTestDevice\n",
"ForeignTestDevice\n",
"UnstableTestDevice\n"
};

void display_help() {
	printf("This is the list of commands.\n");
	printf("All commands should be typed in all lower case.\n");
    printf("\thelp          show this menu of commands\n");
	printf("\tstop          stop dev handler process\n");
	printf("\tconnect       connect specified device\n");
    printf("\tdisconnect    disconnect device from specified socket number\n");
    printf("\tlist          list all currently connected devices and their ports\n");
}

// ********************************** MAIN PROCESS ****************************************** //

void clean_up() {
    printf("Exiting Dev Handler CLI...\n");
    disconnect_all_devices();
    stop_dev_handler();
    printf("Done!\n");
    exit(0);
}

int main() {
    signal(SIGINT, clean_up);

    char nextcmd[MAX_CMD_LEN];
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
        nextcmd[strlen(nextcmd) - 1] = '\0'; // Strip off \n character

        // Compare input string against the available commands
        if (strcmp(nextcmd, "stop") == 0) {
            stop = 0;
        } else if (strcmp(nextcmd, "connect") == 0) {
            int is_device = 0;
            while(!is_device){
                printf("This is the list of devices by name. Type in number on left to use device\n");
                for (int i = 0; i < NUMBER_OF_TEST_DEVICES; i++) {
                    printf("\t%d    %s", i, devices[i]);
                }
                printf("\t5    cancel\n");
                printf("Device name?\n");
                fflush(stdout);
                printf("> ");
                fgets(nextcmd, MAX_CMD_LEN, stdin);
                char * input;
                long int dev_number; 
                dev_number = strtol(nextcmd, &input, 0);
                if(dev_number >= 0 && dev_number < 5){
                    printf("Connecting %s", devices[dev_number]);
                    if(connect_device(devices[dev_number]) == 0){
                        fflush(stdout);
                        printf("Device connected!\n");
                        is_device = 1;
                    } else {
                        printf("Device connection failed!\n");
                        is_device = 1;
                        }
                    } else if (dev_number == 5){
                        printf("Device connecting cancelled \n");
                        is_device = 1;
                    } else {
                        printf("Invalid Device! \n");
                    }
                }
        } else if (strcmp(nextcmd, "disconnect\n") == 0) {
			printf("Socket Number?\n");
            fflush(stdout);
            printf("> ");
            fgets(nextcmd, MAX_CMD_LEN, stdin);
            nextcmd[strlen(nextcmd) - 1] = '\0'; // Strip off \n character
            // TODO: Do nothing if empty input. Currently port_num == 0 if so
            int port_num = atoi(nextcmd);
            if(disconnect_device(port_num) == 0) {
                printf("Device disconnected!\n");
            } else {
                printf("Device disconnect unsuccesful!\n");
            }
            list_devices();
		} else if (strcmp(nextcmd, "list\n") == 0) {
            list_devices();
        } else if (strcmp(nextcmd, "help\n") == 0) {
            display_help();
        }
    }

    clean_up();
    return 0;
}
