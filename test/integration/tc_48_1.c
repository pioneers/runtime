#include "../test.h"

char check_1_output[] =
	"entering AUTO mode\n"
	"Changed devices: 00000000000000000000000000000000\n"
	"Changed params:\n"
	"Requested devices: 00000000000000000000000000000000\n"
	"Requested params:\n"
	"Current Robot Description:\n"
	"\tRUN_MODE = AUTO\n"
	"\tDAWN = CONNECTED\n"
	"\tSHEPHERD = CONNECTED\n"
	"\tGAMEPAD = DISCONNECTED\n"
	"\tSTART_POS = LEFT\n\n"
	"Current Gamepad State:\n"
	"\tPushed Buttons:\n"
	"\tJoystick Positions:\n"
	"\t\tX_LEFT_JOYSTICK = 0.000000\n"
	"\t\tY_LEFT_JOYSTICK = 0.000000\n"
	"\t\tX_RIGHT_JOYSTICK = 0.000000\n"
	"\t\tY_RIGHT_JOYSTICK = 0.000000\n";

char check_2_output[] =
	"robot is in RIGHT start position\n"
	"Changed devices: 00000000000000000000000000000000\n"
	"Changed params:\n"
	"Requested devices: 00000000000000000000000000000000\n"
	"Requested params:\n"
	"Current Robot Description:\n"
	"\tRUN_MODE = AUTO\n"
	"\tDAWN = CONNECTED\n"
	"\tSHEPHERD = CONNECTED\n"
	"\tGAMEPAD = DISCONNECTED\n"
	"\tSTART_POS = RIGHT\n\n"
	"Current Gamepad State:\n"
	"\tPushed Buttons:\n"
	"\tJoystick Positions:\n"
	"\t\tX_LEFT_JOYSTICK = 0.000000\n"
	"\t\tY_LEFT_JOYSTICK = 0.000000\n"
	"\t\tX_RIGHT_JOYSTICK = 0.000000\n"
	"\t\tY_RIGHT_JOYSTICK = 0.000000\n";

int main ()
{
	//setup
	start_test("sanity");
	start_shm();
	start_net_handler();

	//poke the system
	send_run_mode(SHEPHERD_CLIENT, AUTO_MODE);
	print_shm();
	send_start_pos(SHEPHERD_CLIENT, RIGHT_POS);
	print_shm();

	//stop the system
	stop_net_handler();
	stop_shm();
	end_test();

	//do checks
	if (match_part(check_1_output) != 0) {
		exit(1);
	}
	if (match_part(check_2_output) != 0) {
		exit(1);
	}

	return 0;
}
