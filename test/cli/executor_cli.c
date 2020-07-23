#include "../client/executor_client.h"

#define MAX_CMD_LEN 64

void display_help()
{
	printf("This is the main menu.\n");
	printf("All commands should be typed in all lower case.\n");
	printf("\thelp      display this help text\n");
	printf("\tstop      exit the Executor CLI\n");
}

// ********************************** MAIN PROCESS ****************************************** //

void sigint_handler (int signum)
{
	stop_executor();
	exit(0);
}

int main ()
{
	char nextcmd[MAX_CMD_LEN]; //to hold nextline
	int stop = 0;
	
	signal(SIGINT, sigint_handler);
	
	//asks for student code to execute
	printf("Starting Executor CLI...\n");
	printf("Please enter the path to student code file: ");
	fflush(stdout);
	fgets(nextcmd, MAX_CMD_LEN, stdin);
	
	//start executor process
	start_executor(nextcmd);
	
	//command-line loop which prompts user for commands to send to net_handler
	while (!stop) {
		//get the next command
		printf("> ");
		fgets(nextcmd, MAX_CMD_LEN, stdin);
		
		//compare input string against the available commands
		if (strcmp(nextcmd, "stop\n") == 0) {
			stop = 1;
		} else if (strcmp(nextcmd, "help\n") == 0) {
			display_help();
		} else {
			printf("Invalid command %s", nextcmd);
			display_help();
		}
	}
	
	printf("Stopping Executor CLI...\n");
	
	stop_executor();
	
	printf("Done!\n");
	
	return 0;
}