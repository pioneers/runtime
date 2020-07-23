//This is a CLI for net_handler, which prompts user for input to send to net_handler via
//raspi TCP / UDP ports and routes outputs from raspi TCP / UDP ports to this process's standard output
//when used in "CLI" mode, outputs prompt text to standard out; when used in "Test" mode, outputs no
//prompt text to standard out (only output from net_handler)

//requires shared memory process to be running, and net_handler already be compiled and placed in its folder

//TODO: figure out how to initialize the logger from a specified config file instead of the default one in the system
//(maybe a bash script from the main test process to temporarily rename the log file in the system, write the one to use for tests,
//and when the test is done, remove the one used for tests and rename the existing log file back to what it was before the tests)

#include "../client/net_handler_client.h"

#define MAX_CMD_LEN 32

// ********************************** COMMAND-SPECIFIC FUNCTIONS  ****************************** //

void prompt_run_mode()
{
	
}

void prompt_start_pos()
{
	
}

void prompt_challenge_data()
{
	
}

void prompt_device_data()
{
	
}

void display_help()
{
	
}

// ********************************** MAIN PROCESS ****************************************** //

void sigint_handler (int signum)
{
	printf("Stopping net handler CLI...\n");
	stop_net_handler();	
	printf("Done!\n");
	exit(0);
}

int main (int argc, char **argv)
{
	struct sockaddr_in udp_servaddr; //for holding server address of UDP connection with raspi
	char nextcmd[MAX_CMD_LEN]; //to hold nextline
	int stop = 0;
	
	signal(SIGINT, sigint_handler);
	
	printf("Starting net handler CLI...\n");
	fflush(stdout);
	
	//start the net handler and connect all of its output locations to file descriptors in this process
	start_net_handler(&udp_servaddr);
	
	sleep(1); //any logs will be extracted and printed during this sleep, before asking for first command
	
	//command-line loop which prompts user for commands to send to net_handler
	while (!stop) {
		//get the next command
		printf("> ");
		fgets(nextcmd, MAX_CMD_LEN, stdin);
		
		//compare input string against the available commands
		if (strcmp(nextcmd, "stop\n") == 0) {
			stop = 1;
		} else if (strcmp(nextcmd, "run mode\n") == 0) {
			prompt_run_mode();
		} else if (strcmp(nextcmd, "start pos\n") == 0) {
			prompt_start_pos();
		} else if (strcmp(nextcmd, "challenge data\n") == 0) {
			prompt_challenge_data();
		} else if (strcmp(nextcmd, "device data\n") == 0) {
			prompt_device_data();
		} else if (strcmp(nextcmd, "help\n") == 0) {
			display_help();
		} else {
			printf("Invalid command %s", nextcmd);
		}
	}
	
	printf("Stopping net handler CLI...\n");
	
	stop_net_handler();
	
	printf("Done!\n");
	
	return 0;
}