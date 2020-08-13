#include "../client/executor_client.h"

#define MAX_CMD_LEN 64

char student_code[MAX_CMD_LEN] = {0};
char challenge_code[MAX_CMD_LEN] = {0};

void display_help()
{
	printf("This is the main menu.\n");
	printf("All commands should be typed in all lower case.\n");
	printf("\tstudent code      change the student code file\n");
	printf("\tchallenge code    change the challenge code file\n");
	printf("\thelp              display this help text\n");
	printf("\texit              exit the Executor CLI\n");
}

// ********************************** MAIN PROCESS ****************************************** //

void change_student_code() {
	printf("Please enter the name of the file to use as student code (do not enter the \".py\"): ");
	fgets(student_code, MAX_CMD_LEN, stdin);
	stop_executor();
	start_executor(student_code, challenge_code);
}

void change_challenge_code() {
	printf("Please enter the name of the file to use as challenge code (do not enter the \".py\"): ");
	fgets(challenge_code, MAX_CMD_LEN, stdin);
	stop_executor();
	start_executor(student_code, challenge_code);
}

int main ()
{
	char nextcmd[MAX_CMD_LEN]; //to hold nextline
	int stop = 0;
		
	printf("Starting Executor CLI...\n");
    strcpy(student_code, "studentcode");
    strcpy(challenge_code, "challenges");
	
	//start executor process
	start_executor(student_code, challenge_code);
	
	//command-line loop which prompts user for commands to send to net_handler
	while (!stop) {
		//get the next command
		printf("> ");
		fgets(nextcmd, MAX_CMD_LEN, stdin);
		
		//compare input string against the available commands
		if (strcmp(nextcmd, "exit\n") == 0) {
			stop = 1;
		} 
		else if (strcmp(nextcmd, "help\n") == 0) {
			display_help();
		} 
		else if (strcmp(nextcmd, "student code\n") == 0) {
			change_student_code();
		} 
		else if (strcmp(nextcmd, "challenge code\n") == 0) {
			change_challenge_code();
		}
		else {
			printf("Invalid command %s", nextcmd);
			display_help();
		}
	}
	
	printf("Exiting Executor CLI...\n");
	
	stop_executor();
	
	printf("Done!\n");
	
	return 0;
}
