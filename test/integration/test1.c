#include "../test.h"

char expected_output[] = "adf";

int main ()
{
	//setup
	start_test("sanity_test");
	start_shm();
	fprintf(stderr, "hi\n");
	start_net_handler();
	fprintf(stderr, "hi\n");
	
	//poke the system
	send_run_mode(SHEPHERD_CLIENT, AUTO_MODE);
	fprintf(stderr, "hi\n");
	print_shm();
	fprintf(stderr, "hi\n");
	send_start_pos(SHEPHERD_CLIENT, RIGHT_POS);
	fprintf(stderr, "hi\n");
	print_shm();
	fprintf(stderr, "hi\n");
	
	//stop the system
	stop_net_handler();
	stop_shm();
	end_test();

	if (match_all(expected_output) != 0) {
		exit(1);
	}
	
	return 0;
}