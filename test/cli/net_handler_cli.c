#include "../client/net_handler_client.h"

#define MAX_CMD_LEN 32

// ********************************** COMMAND-SPECIFIC FUNCTIONS  ****************************** //

void prompt_run_mode()
{
	int client = SHEPHERD_CLIENT;
	int mode = IDLE_MODE;
	char nextcmd[MAX_CMD_LEN];
	
	//get client to send as
	while (1) {
		printf("Send as DAWN or SHEPHERD: ");
		fgets(nextcmd, MAX_CMD_LEN, stdin);
		if (strcmp(nextcmd, "dawn\n") == 0) {
			client = DAWN_CLIENT;
			break;
		} else if (strcmp(nextcmd, "shepherd\n") == 0) {
			client = SHEPHERD_CLIENT;
			break;
		} else if (strcmp(nextcmd, "abort\n") == 0) {
			return;
		} else {
			printf("Invalid response to prompt: %s", nextcmd);
		}
	}
	
	//get mode to send
	while (1) {
		printf("Send mode IDLE, AUTO, or TELEOP: ");
		fgets(nextcmd, MAX_CMD_LEN, stdin);
		if (strcmp(nextcmd, "idle\n") == 0) {
			mode = IDLE_MODE;
			break;
		} else if (strcmp(nextcmd, "auto\n") == 0) {
			mode = AUTO_MODE;
			break;
		} else if (strcmp(nextcmd, "teleop\n") == 0) {
			mode = TELEOP_MODE;
			break;
		} else if (strcmp(nextcmd, "abort\n") == 0) {
			return;
		} else {
			printf("Invalid response to prompt: %s", nextcmd);
		}
	}
	
	//send
	printf("Sending Run Mode message!\n\n");
	send_run_mode(client, mode);
}

void prompt_start_pos()
{
	int client = SHEPHERD_CLIENT;
	int pos = LEFT_POS;
	char nextcmd[MAX_CMD_LEN];
	
	//get client to send as
	while (1) {
		printf("Send as DAWN or SHEPHERD: ");
		fgets(nextcmd, MAX_CMD_LEN, stdin);
		if (strcmp(nextcmd, "dawn\n") == 0) {
			client = DAWN_CLIENT;
			break;
		} else if (strcmp(nextcmd, "shepherd\n") == 0) {
			client = SHEPHERD_CLIENT;
			break;
		} else if (strcmp(nextcmd, "abort\n") == 0) {
			return;
		} else {
			printf("Invalid response to prompt: %s", nextcmd);
		}
	}
	
	//get pos to send
	while (1) {
		printf("Send pos LEFT or RIGHT: ");
		fgets(nextcmd, MAX_CMD_LEN, stdin);
		if (strcmp(nextcmd, "left\n") == 0) {
			pos = LEFT_POS;
			break;
		} else if (strcmp(nextcmd, "right\n") == 0) {
			pos = RIGHT_POS;
			break;
		} else {
			printf("Invalid response to prompt: %s", nextcmd);
		}
	}
	
	//send
	printf("Sending Start Pos message!\n\n");
	send_start_pos(client, pos);
}

void prompt_challenge_data()
{
	
}

void prompt_device_data()
{
	
}

void display_help()
{
	printf("This is the main menu. Type one of the following commands to send a message to net_handler.\n");
	printf("Once you type one of the commands and hit \"enter\", follow on-screen instructions.\n");
	printf("At any point while following the instructions, type \"abort\" to go back to this main menu.\n");
	printf("All commands should be typed in all lower case (including when being prompted to send messages)\n");
	printf("\trun mode           send a Run Mode message\n");
	printf("\tstart pos          send a Start Pos message\n");
	printf("\tchallenge data     send a Challenge Data message\n");
	printf("\tdevice data        send a Device Data message (send a subscription request)\n");
	printf("\thelp               display this help text\n");
	printf("\tstop               exit the Net Handler CLI\n");
}

// ********************************** MAIN PROCESS ****************************************** //

void sigint_handler (int signum)
{
	stop_net_handler();
	remove(CHALLENGE_SOCKET);
	exit(0);
}

int main ()
{
	char nextcmd[MAX_CMD_LEN]; //to hold nextline
	int stop = 0;
	
	signal(SIGINT, sigint_handler);
	
	printf("Starting Net Handler CLI...\n");
	fflush(stdout);
	
	//start the net handler and connect all of its output locations to file descriptors in this process
	start_net_handler();
	
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
			display_help();
		}
	}
	
	printf("Stopping Net Handler CLI...\n");
	
	stop_net_handler();
	
	printf("Done!\n");
	
	return 0;
}
