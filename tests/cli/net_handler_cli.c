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

void prompt_device_data ()
{
	char nextcmd[MAX_CMD_LEN];
	char dev_names[DEVICES_LENGTH][32]; //for holding the names of the devices
	int num_devices = 0;
	long int temp;
	dev_data_t data[MAX_DEVICES];
	device_t *curr_dev;
	
	//first get the list of device names
	for (int i = 0; i < DEVICES_LENGTH; i++) {
		if ((curr_dev = get_device((uint8_t) i)) != NULL) {
			strcpy(dev_names[i], curr_dev->name);
		} else {
			strcpy(dev_names[i], "");
		}
	}
	
	//enter prompt loop
	while (1) {
		//subscribe to another device?
		printf("Type \"done\" if ready to send message; type anything else to continue: ");
		fgets(nextcmd, MAX_CMD_LEN, stdin);
		if (strcmp(nextcmd, "done\n") == 0) {
			break;
		}
		
		//ask for UID
		while (1) {
			printf("Enter the UID of the device, in base 10: ");
			fgets(nextcmd, MAX_CMD_LEN, stdin);
			
			if (strcmp(nextcmd, "abort\n") == 0) {
				return;
			}
			temp = strtol(nextcmd, NULL, 10);
			if (temp < 0) {
				printf("Input is not a positive number: %ld\n", temp);
				continue;
			} else {
				data[num_devices].uid = (uint64_t) temp;
				break;
			}
		}
		
		//ask for device type
		data[num_devices].name = NULL;
		printf("Specify the device type, one of the following (exactly as they appear in the list):\n");
		for (int i = 0; i < DEVICES_LENGTH; i++) {
			if (strcmp(dev_names[i], "") != 0) {
				printf("\t%s\n", dev_names[i]);
			}
		}
		while (1) {	
			printf("Enter the device type: ");
			fgets(nextcmd, MAX_CMD_LEN, stdin);
			if (strcmp(nextcmd, "abort\n") == 0) {
				return;
			}
			nextcmd[strlen(nextcmd) - 1] = '\0'; //get rid of the newline at the end of nextcmd
			
			//check to see if valid device
			for (int i = 0; i < DEVICES_LENGTH; i++) {
				if (strcmp(dev_names[i], "") == 0) {
					continue;
				}
				if (strcmp(nextcmd, dev_names[i]) == 0) {
					data[num_devices].name = malloc(strlen(dev_names[i]));
					strcpy(data[num_devices].name, dev_names[i]);
					break;
				}
			}
			if (data[num_devices].name == NULL) {
				printf("Input is not a valid device name: %s\n", nextcmd);
			} else {
				break;
			}
		}
		
		//fetch parameters and prompt user "y" or "n" on each one
		curr_dev = get_device(device_name_to_type(data[num_devices].name));
		data[num_devices].params = 0;
		printf("Now specify whether to subscribe to given param or not.\n");
		printf("If a subscription is desired, type lowercase letter \"y\", if not type \"n\"\n");
		for (int i = 0; i < curr_dev->num_params; i++) {
			while (1) {
				printf("\t%s? ", curr_dev->params[i].name);
				fgets(nextcmd, MAX_CMD_LEN, stdin);
				if (strcmp(nextcmd, "abort\n") == 0) {
					return;
				}
				if (strcmp(nextcmd, "y\n") == 0) {
					data[num_devices].params |= (1 << i);
					break;
				} else if (strcmp(nextcmd, "n\n") == 0) {
					break;
				} else {
					printf("Input is not \"y\" or \"n\": %s\n", nextcmd);
				}
			}
		}
		num_devices++;
	}
	
	//send
	printf("Sending Device Data message!\n\n");
	send_device_data(data, num_devices);
	
	//free everything
	for (int i = 0; i < num_devices; i++) {
		free(data[i].name);
	}
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
