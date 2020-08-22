#include "../test.h"

/**
 * This test checks to make sure that device subscriptions made to nonexistent devices
 * from Dawn are rejected by shared memory and an error message is sent back to Dawn.
 */

char check_output_1[] = "no device at dev_uid = 50, sub request failed\n";

char check_output_2[] = "recv_new_msg: Invalid device subscription, device uid 50 is invalid\n";

char check_output_3[] = "no device at dev_uid = 100, sub request failed\n";

char check_output_4[] = "recv_new_msg: Invalid device subscription, device uid 100 is invalid\n";

int main() {
	// setup
	start_test("nonexistent device subscription");
	start_shm();
	start_net_handler();

	// poke
	dev_subs_t data1 = { .uid = 50, .name = "ServoControl", .params = 0b11 };
	dev_subs_t data2 = { .uid = 100, .name = "LimitSwitch", .params = 0b101 };
	dev_subs_t data_total[2] = { data1, data2 };
	send_device_subs(data_total, 2);

	// stop
	stop_net_handler();
	stop_shm();
	end_test();

	// check output
	in_rest_of_output(check_output_1);
	in_rest_of_output(check_output_2);
	in_rest_of_output(check_output_3);
	in_rest_of_output(check_output_4);
	return 0;
}
