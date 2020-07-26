#include "../client/shm_client.h"

#define MAX_CMD_LEN 32

void display_help()
{
	printf("This is the main menu.\n");
	printf("All commands should be typed in all lower case.\n");
	printf("\tprint     print the current state of shared memory\n");
	printf("\thelp      display this help text\n");
	printf("\texit      exit the Shared Memory CLI\n");
}

// ********************************** MAIN PROCESS ****************************************** //

void sigint_handler (int signum)
{
	stop_shm();
	exit(0);
}

int main ()
{
	char nextcmd[MAX_CMD_LEN]; //to hold nextline
	int stop = 0;
	
	signal(SIGINT, sigint_handler);
	
	printf("Starting Shared Memory CLI...\n");
	fflush(stdout);
	
	//start shm process
	start_shm();
	
	//command-line loop which prompts user for commands to send to net_handler
	while (!stop) {
		//get the next command
		printf("> ");
		fgets(nextcmd, MAX_CMD_LEN, stdin);
		
		//compare input string against the available commands
		if (strcmp(nextcmd, "exit\n") == 0) {
			stop = 1;
		} else if (strcmp(nextcmd, "print\n") == 0) {
			print_shm();
		} else if (strcmp(nextcmd, "help\n") == 0) {
			display_help();
		} else {
			printf("Invalid command %s", nextcmd);
			display_help();
		}
	}
	
	printf("Exiting Shared Memory CLI...\n");
	
	stop_shm();
	
	printf("Done!\n");
	
	return 0;
}