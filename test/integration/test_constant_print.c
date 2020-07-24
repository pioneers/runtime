#include "../test.h"

char check_output_1[] = "";

char check_output_2[] = "";

int main ()
{
	//set everything up
	start_test("constant print");
	start_shm();
	start_net_handler();
	start_executor("../student_code/constant_print.py");

	//poke the system
	send_start_pos(SHEPHERD_CLIENT, RIGHT_POS);
	print_shm();
	send_run_mode(SHEPHERD_CLIENT, AUTO_MODE);
	sleep(7);
	send_run_mode(SHEPHERD_CLIENT, IDLE_MODE);
	print_shm();

	//stop all the tests
	stop_executor();
	stop_net_handler();
	stop_shm();
	end_test();

	//check outputs
	return 0;
}
