#include "../test.h"

char check_output_1[] = "no device at dev_uid = 50, sub request failed\n";

char check_output_2[] = "Invalid device subscription: device uid 50 is invalid\n";

char check_output_3[] = "no device at dev_uid = 100, sub request failed\n";

char check_output_4[] = "Invalid device subscription: device uid 100 is invalid\n";

int main ()
{
	//setup
	start_test("nonexistent device subscription");
	start_shm();
	start_net_handler();

	//poke
	dev_data_t data1 = { .uid = 50, .name = "ServoControl", .params = 0b11 };
	dev_data_t data2 = { .uid = 100, .name = "LimitSwitch", .params = 0b101 };
	dev_data_t data_total[2] = { data1, data2 };
	send_device_data(data_total, 2);

	//stop
	stop_net_handler();
	stop_shm();
	end_test();

	//check output
	match_part(check_output_1);
	match_part(check_output_2);
	match_part(check_output_3);
	match_part(check_output_4);
	return 0;
}
