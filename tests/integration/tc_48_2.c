#include "../test.h"

char check_output_1[] = "New mode 1\n";

char check_output_2[] = "Autonomous setup has begun!\n";

char check_output_3[] = "autonomous printing again\n";

char check_output_4[] = "\tRUN_MODE = AUTO\n";

char check_output_5[] = "In KILL subprocess\n";

char check_output_6[] = "New mode 0\n";

int main ()
{
	//set everything up
	start_test("constant print");
	start_shm();
	start_net_handler();
	start_executor("constant_print");

	//poke the system
	send_start_pos(SHEPHERD_CLIENT, RIGHT_POS);
	send_run_mode(SHEPHERD_CLIENT, AUTO_MODE);
	sleep(1);
	print_shm();
	sleep(2);
	send_run_mode(SHEPHERD_CLIENT, IDLE_MODE);

	//stop all the tests
	stop_executor();
	stop_net_handler();
	stop_shm();
	end_test();

	//check outputs
	if (match_part(check_output_1) != 0) {
		exit(1);
	}
	if (match_part(check_output_2) != 0) {
		exit(1);
	}
	if (match_part(check_output_3) != 0) {
		exit(1);
	}
	if (match_part(check_output_4) != 0) {
		exit(1);
	}
	if (match_part(check_output_3) != 0) {
		exit(1);
	}
	if (match_part(check_output_5) != 0) {
		exit(1);
	}
	if (match_part(check_output_6) != 0) {
		exit(1);
	}

	return 0;
}
