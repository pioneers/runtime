#include "../test.h"

char check_output_1[] =
	"Current Robot Description:\n"
	"\tRUN_MODE = IDLE\n"
	"\tDAWN = CONNECTED\n"
	"\tSHEPHERD = CONNECTED\n"
	"\tGAMEPAD = CONNECTED\n"
	"\tSTART_POS = LEFT\n\n"
	"Current Gamepad State:\n"
	"\tPushed Buttons:\n"
	"\t\tA_BUTTON\n"
	"\t\tL_TRIGGER\n"
	"\t\tDOWN_DPAD\n"
	"\tJoystick Positions:\n"
	"\t\tX_LEFT_JOYSTICK = -0.100000\n"
	"\t\tY_LEFT_JOYSTICK = 0.000000\n"
	"\t\tX_RIGHT_JOYSTICK = 0.100000\n"
	"\t\tY_RIGHT_JOYSTICK = 0.990000\n";

char check_output_2[] =
	"Raspi IP is 127.0.0.1:9000\n"
	"Device No. 0:\ttype = CustomData, uid = 0, itype = 32\n";

int main ()
{
	//setup
	start_test("UDP; no devices connected");
	start_shm();
	start_net_handler();

	//poke system
	uint32_t buttons = (1 << A_BUTTON) | (1 << L_TRIGGER) | (1 << DOWN_DPAD);
	float joystick_vals[] = { -0.1, 0.0, 0.1, 0.99 };
	send_gamepad_state(buttons, joystick_vals);
	print_shm();
	print_next_dev_data();

	//stop all processes
	stop_net_handler();
	stop_shm();
	end_test();

	//check outputs
	in_rest_of_output(check_output_1);
	in_rest_of_output(check_output_2);
	return 0;
}
