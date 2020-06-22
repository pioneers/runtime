#include "net_handler.h"

//IP addresses of Dawn and Shepherd and port numbers

void sigint_handler (int sig_num)
{
	log_runtime(INFO, "quitting net_handler");
	if (robot_desc_read(SHEPHERD_TARGET) == CONNECTED) {
		stop_tcp_suite(SHEPHERD_TARGET);
	}
	if (robot_desc_read(DAWN_TARGET) == CONNECTED) {
		stop_tcp_suite(DAWN_TARGET);
		stop_udp_suite();
	}
	shm_aux_stop(NET_HANDLER);
	shm_stop(NET_HANDLER);
	logger_stop(NET_HANDLER);
}

int main ()
{
	//setup
	shm_aux_init(NET_HANDLER);
	shm_init(NET_HANDLER);
	logger_init(NET_HANDLER);
	signal(SIGINT, sigint_handler);
	
	while (1) {
		//TODO: open sockets and wait for connections
		if (/* Shepherd connected */) {
			start_tcp_suite(SHEPHERD_TARGET);
		}
		
		if (/* Shepherd disconnected */) {
			stop_tcp_suite(SHEPHERD_TARGET);
		}
		
		if (/* Dawn connected */) {
			start_tcp_suite(DAWN_TARGET);
			start_udp_suite();
		}
		
		if (/* Dawn disconnected */) {
			stop_tcp_suite(DAWN_TARGET);
			stop_udp_suite();
		}
		
		//TODO: handle dropped connections and opening new sockets
	}
	
	return 0;
}