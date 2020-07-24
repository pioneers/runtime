#include "../client/net_handler_client.h"

#define MAX_CMD_LEN 32

// ********************************** COMMAND-SPECIFIC FUNCTIONS  ****************************** //

void prompt_run_mode ()
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

void prompt_start_pos ()
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
		} else if (strcmp(nextcmd, "abort\n") == 0) {
			return;
		} else {
			printf("Invalid response to prompt: %s", nextcmd);
		}
	}
	
	//send
	printf("Sending Start Pos message!\n\n");
	send_start_pos(client, pos);
}

void prompt_gamepad_state ()
{
	uint32_t buttons = 0;
	float joystick_vals[4];
	char nextcmd[MAX_CMD_LEN];
	char **list_of_names;
	char button_name[16];
	int button_ix = 0;
	int set_joysticks = 0;
	
	//get which buttons are pressed
	printf("Specify which of the following buttons are pressed:\n");
	list_of_names = get_button_names();
	for (int i = 0; i < NUM_GAMEPAD_BUTTONS; i++) {
		printf("\t%s\n", list_of_names[i]);
	}
	printf("After you have finished setting buttons, type \"done\"\n");
	printf("If you accidentally set a button, you can unset the button by typing its name again\n");
	
	while (1) {
		printf("Set/unset this button: ");
		fgets(nextcmd, MAX_CMD_LEN, stdin);
		
		//check for these first
		if (strcmp(nextcmd, "done\n") == 0) {
			break;
		} else if (strcmp(nextcmd, "abort\n") == 0) {
			return;
		}
		
		//get the index of the button that was specified
		for (button_ix = 0; button_ix < NUM_GAMEPAD_BUTTONS; button_ix++) {
			//this logic appends a newline to the end of the button name
			strcpy(button_name, list_of_names[button_ix]);
			if (strcmp(nextcmd, strcat(button_name, "\n")) == 0) {
				break;
			}
		}
		if (button_ix == NUM_GAMEPAD_BUTTONS) {
			printf("Not a button: %s\n", nextcmd);
			continue;
		}
		
		//otherwise set that bit in buttons
		if (buttons & (1 << button_ix)) {
			buttons &= ~(1 << button_ix);
			printf("\tOK, unset button %s\n", list_of_names[button_ix]);
		} else {
			buttons |= (1 << button_ix);
			printf("\tOK, set button %s\n", list_of_names[button_ix]);
		}
	}
	
	//get the joystick values to send
	printf("Specify the values of the four joysticks (values -1.0 <= val <= 1.0):\n");
	list_of_names = get_joystick_names();
	for (int i = 0; i < 4; i++) {
		printf("\t%s\n", list_of_names[i]);
	}
	
	while (set_joysticks != 4) {
		printf("Set the value for %s: ", list_of_names[set_joysticks]);
		fgets(nextcmd, MAX_CMD_LEN, stdin);
		
		if (strcmp(nextcmd, "abort\n") == 0) {
			return;
		}
		
		errno = 0;
		if (((joystick_vals[set_joysticks] = strtof(nextcmd, NULL)) == 0) && errno != 0) {
			printf("Input is not a float: %s\n", nextcmd);
		}
		if (joystick_vals[set_joysticks] >= 1.0 || joystick_vals[set_joysticks] <= -1.0) {
			printf("Input is outside of range of joystick: %s\n", nextcmd);
			continue;
		}
		printf("\tOk! %s set to %f\n", list_of_names[set_joysticks], joystick_vals[set_joysticks]);
		set_joysticks++;
	}
	
	//send
	printf("Sending Gamepad State message!\n\n");
	send_gamepad_state(buttons, joystick_vals);
}

void prompt_challenge_data ()
{
	int client = SHEPHERD_CLIENT;
	char nextcmd[MAX_CMD_LEN];
	char **inputs = malloc(sizeof(char *) * NUM_CHALLENGES);
	
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
	
	//get challenge inputs to send
	for (int i = 1; i <= NUM_CHALLENGES; i++) {
		//TODO: if we ever put the current names of challenges in runtime_util, make this print better
		printf("Provide input for challenge %d of %d: ", i, NUM_CHALLENGES); 
		fgets(nextcmd, MAX_CMD_LEN, stdin);
		
		if (strcmp(nextcmd, "abort\n") == 0) {
			return;
		}
		
		//we need to do this because nextcmd has a newline at the end
		inputs[i - 1] = malloc(strlen(nextcmd) - 1);
		for (int j = 0; j < strlen(nextcmd) - 1; j++) {
			inputs[i - 1][j] = nextcmd[j];
		}
	}
	
	//send
	printf("Sending Challenge Data message!\n\n");
	send_challenge_data(client, inputs);
	
	//free pointers
	for (int i = 0; i < NUM_CHALLENGES; i++) {
		free(inputs[i]);
	}
	free(inputs);
}

//TODO: fill in this prompt
void prompt_device_data ()
{
	//char nextcmd[MAX_CMD_LEN];
	
	//ask for UID
	//ask for device type
	//fetch parameters and prompt user "y" or "n" on each one
	//repeat
	
	//send
	//free pointers
}

void display_help()
{
	printf("This is the main menu. Type one of the following commands to send a message to net_handler.\n");
	printf("Once you type one of the commands and hit \"enter\", follow on-screen instructions.\n");
	printf("At any point while following the instructions, type \"abort\" to go back to this main menu.\n");
	printf("All commands should be typed in all lower case (including when being prompted to send messages)\n");
	printf("\trun mode           send a Run Mode message\n");
	printf("\tstart pos          send a Start Pos message\n");
	printf("\tgamepad state      send a Gamepad State message\n");
	printf("\tchallenge data     send a Challenge Data message\n");
	printf("\tdevice data        send a Device Data message (send a subscription request)\n");
	printf("\tview device data   view the next UDP packet sent to Dawn containing most recent device data\n");
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
		} else if (strcmp(nextcmd, "gamepad state\n") == 0) {
			prompt_gamepad_state();
		} else if (strcmp(nextcmd, "challenge data\n") == 0) {
			prompt_challenge_data();
		} else if (strcmp(nextcmd, "device data\n") == 0) {
			prompt_device_data();
		} else if (strcmp(nextcmd, "view device data\n") == 0) {
			print_next_dev_data();
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
