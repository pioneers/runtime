#include "net_util.h"
#include "shepherd_conn.h"

int listening_socket_setup (int *sockfd)
{
	struct sockaddr_in serv_addr;
		
	//create socket
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		error("socket: failed to create listening socket");
		logger_stop(NET_HANDLER);
		return 1;
	} else {
		log_runtime(DEBUG, "socket: successfully created listening socket");
	}
	
	//set the elements of serv_addr
	memset(&serv_addr, '\0', sizeof(struct sockaddr_in));  //initialize everything to 0
	serv_addr.sin_family = AF_INET;                        //use IPv4
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);         //use any IP interface on raspi
	serv_addr.sin_port = htons(RASPI_PORT);                //assign a port number
	
	//bind socket to well-known IP_addr:port
	if ((bind(*sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in))) != 0) {
		error("bind: failed to bind listening socket to raspi port");
		logger_stop(NET_HANDLER);
		close(*sockfd);
		return 1;
	} else {
		log_runtime(DEBUG, "bind: successfully bound listening socket to raspi port");
	}
	
	//set the socket to be in listening mode (since the robot is the server)
	if ((listen(*sockfd, 2)) != 0) {
		error("listen: failed to set listening socket to listen mode");
		logger_stop(NET_HANDLER);
		close(*sockfd);
		return 1;
	} else {
		log_runtime(DEBUG, "listen: successfully set listening socket to listen mode");
	}
	return 0;
}

int is_shepherd (struct sockaddr_in *cli_addr)
{
	//check if the client requesting connection is shepherd, and if shepherd is connected already
	if (cli_addr->sin_family != AF_INET
			|| cli_addr->sin_addr.s_addr != inet_addr(SHEPHERD_ADDR)
			|| cli_addr->sin_addr.s_addr != htons(SHEPHERD_PORT)
			|| robot_desc_read(SHEPHERD) == CONNECTED) {
		return 0;
	}
	return 1;
}

int is_dawn (struct sockaddr_in *cli_addr)
{
	//check if the client requesting connection is dawn, and if dawn is connected already
	if (cli_addr->sin_family != AF_INET
			|| cli_addr->sin_addr.s_addr != inet_addr(DAWN_ADDR)
			|| cli_addr->sin_addr.s_addr != htons(DAWN_PORT)
			|| robot_desc_read(DAWN) == CONNECTED) {
		return 0;
	}
	return 1;
}

void sigint_handler (int sig_num)
{
	log_runtime(INFO, "stopping net_handler");
	if (robot_desc_read(SHEPHERD) == CONNECTED) {
		stop_shepherd_conn();
	}
	if (robot_desc_read(DAWN) == CONNECTED) {
		//stop_dawn_conn();
		//stop_dawn_udp();
	}
	shm_aux_stop(NET_HANDLER);
	shm_stop(NET_HANDLER);
	logger_stop(NET_HANDLER);
	//sockfd is automatically closed when process terminates
}

// ******************************************* MAIN ROUTINE ******************************* //

int main ()
{
	int sockfd = -1, connfd = -1;
	struct sockaddr_in cli_addr;
	socklen_t cli_addr_len = sizeof(struct sockaddr_in);
	
	//setup
	logger_init(NET_HANDLER);
	signal(SIGINT, sigint_handler);
	if (listening_socket_setup(&sockfd) != 0) {
		logger_stop(NET_HANDLER);
		if (sockfd != -1) {
			close(sockfd);
		}
		return 1;
	}
	shm_aux_init(NET_HANDLER);
	shm_init(NET_HANDLER);
	
	//TODO: incorporate a bit more security into this but for barebones this is fine
	while (1) {
		//wait for a client to make a request to the robot, and accept it
		if ((connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_addr_len)) < 0) {
			error("accept: listen socket failed to accept a connection");
			continue;
		}
		
		//if the incoming request is dawn or shepherd, start the appropriate threads
		if (is_shepherd(&cli_addr)) {
			start_shepherd_conn(connfd);
		} else if (is_dawn(&cli_addr)) {
			//start_dawn_conn(connfd);
			//start_dawn_udp();
		}
	}
	
	return 0;
}