#include "../client/dev_handler_client.h"

#define MAX_CMD_LEN 64

void display_help()
{
	printf("This is the main menu.\n");
	printf("All commands should be typed in all lower case.\n");
	printf("\tstop dev handler      stop dev handler process\n");
	printf("\tconnect device      connect specified device\n");
    printf("\tdisconnect device      disconnect device from specified port\n");
    printf("\tlist devices      list all currently connected devices and their ports\n");
}

// ********************************** MAIN PROCESS ****************************************** //

void sigint_handler(int signum) 
{
    stop_dev_handler();
    exit(0);
}

int main() 
{
    char nextcmd[MAX_CMD_LEN];
    int stop = 1;

    signal(SIGINT, sigint_handler);

    //asks for student code to execute
	printf("Starting Dev Handler CLI...\n");
    fflush(stdout);
    printf("Please enter device handler command:\n ");
    fflush(stdout);
    start_dev_handler();

    while(stop)
    {
        //get the next command
		printf("> ");
		fgets(nextcmd, MAX_CMD_LEN, stdin);

        //compare input string against the available commands
		if (strcmp(nextcmd, "stop dev handler\n") == 0) {
			stop = 0;
		} else if (strcmp(nextcmd, "connect device\n") == 0) {
			printf("Device name?");
            fflush(stdout);
            printf("> ");
		    fgets(nextcmd, MAX_CMD_LEN, stdin);
            connect_device(nextcmd);
		} else if (strcmp(nextcmd, "disconnect device\n") == 0) {
			printf("Port Number?");
            fflush(stdout);
            printf("> ");
		    fgets(nextcmd, MAX_CMD_LEN, stdin);
            int port_num = atoi(nextcmd);
            disconnect_device(port_num);
		} else if (strcmp(nextcmd, "list devices\n") == 0) {
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
