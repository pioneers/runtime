#include "../test.h"

char check_output_1[] = "Autonomous setup has begun!\n";

char check_output_2[] = "autonomous printing again\n";

char check_output_3[] = "\tRUN_MODE = AUTO\n";

char check_output_4[] = "\tRUN_MODE = IDLE\n";

int main ()
{
	//set everything up
	start_test("constant print");
	start_shm();
	start_net_handler();
	start_executor("constant_print");

	//poke the system
	send_start_pos(SHEPHERD, RIGHT);
	send_run_mode(SHEPHERD, AUTO);
	sleep(1);
	print_shm();
	sleep(2);
	send_run_mode(SHEPHERD, IDLE);
	print_shm();

	//stop all the tests
	stop_executor();
	stop_net_handler();
	stop_shm();
	end_test();

	//check outputs
	match_part(check_output_1);
	match_part(check_output_2);
	match_part(check_output_3);
	match_part(check_output_2);
	match_part(check_output_4);
	return 0;
}
