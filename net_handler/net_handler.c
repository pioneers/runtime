#include "net_util.h"
#include "tcp_conn.h"
#include "udp_conn.h"

/*
* Sets up TCP listening socket on raspberry pi.
* Creates the socket, binds it to the raspi's well-known address and port, and puts it in listen mode.
* Arguments:
*    - int *sockfd: pointer to integer which will store the litening socket descriptor upon successful return
* Return:
*    - 0: all steps completed successfully
*    - 1: listening socket setup failed
*/
int listening_socket_setup (int *sockfd)
{
	struct sockaddr_in serv_addr;
		
	//create socket
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		log_printf(ERROR, "failed to create listening socket");
		logger_stop(NET_HANDLER);
		return 1;
	} else {
		log_printf(DEBUG, "socket: successfully created listening socket");
	}
	
	//set the socket option SO_REUSEPORT on so that if raspi terminates and restarts it can immediately reopen the same port
	int optval = 1;
	if ((setsockopt(*sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int))) != 0) {
		perror("setsockopt");
		log_printf(ERROR, "failed to set listening socket for reuse of port");
	}
	
	//set the elements of serv_addr
	memset(&serv_addr, '\0', sizeof(struct sockaddr_in));  //initialize everything to 0
	serv_addr.sin_family = AF_INET;                        //use IPv4
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);         //use any IP interface on raspi
	serv_addr.sin_port = htons(RASPI_PORT);                //assign a port number
	
	//bind socket to well-known IP_addr:port
	if ((bind(*sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in))) != 0) {
		perror("bind");
		log_printf(ERROR, "failed to bind listening socket to raspi port");
		logger_stop(NET_HANDLER);
		close(*sockfd);
		return 1;
	} else {
		log_printf(DEBUG, "bind: successfully bound listening socket to raspi port");
	}
	
	//set the socket to be in listening mode (since the robot is the server)
	if ((listen(*sockfd, 2)) != 0) {
		perror("listen");
		log_printf(ERROR, "failed to set listening socket to listen mode");
		logger_stop(NET_HANDLER);
		close(*sockfd);
		return 1;
	} else {
		log_printf(DEBUG, "listen: successfully set listening socket to listen mode");
	}
	return 0;
}

/*
* Check to see if requesting client is indeed Shepherd
* Arguments:
*    - struct sockaddr_in *cli_addr: pointer to struct sockaddr_in containing the address and port of requesting client
* Return:
*    - 0 if cli_addr is not Shepherd
*    - 1 if cli_addr is Shepherd
*/
int is_shepherd (struct sockaddr_in *cli_addr)
{
	//check if the client requesting connection is shepherd, and if shepherd is connected already
	if (cli_addr->sin_family != AF_INET
			|| cli_addr->sin_port != htons(SHEPHERD_PORT)
			|| robot_desc_read(SHEPHERD) == CONNECTED) {
		return 0;
	}
	return 1;
}

/*
* Check to see if requesting client is indeed Dawn
* Arguments:
*    - struct sockaddr_in *cli_addr: pointer to struct sockaddr_in containing the address and port of requesting client
* Return:
*    - 0 if cli_addr is not Dawn
*    - 1 if cli_addr is Dawn
*/
int is_dawn (struct sockaddr_in *cli_addr)
{
	//check if the client requesting connection is dawn, and if dawn is connected already
	if (cli_addr->sin_family != AF_INET
			|| cli_addr->sin_port != htons(DAWN_PORT)
			|| robot_desc_read(DAWN) == CONNECTED) {
		return 0;
	}
	return 1;
}

/*
* Handles SIGINT being sent to the process by closing connections and closing shm_aux, shm, and logger.
* Arguments:
*    - int sig_num: signal that caused this handler to execute (will always be SIGINT in this case)
*/
void sigint_handler (int sig_num)
{
	log_printf(DEBUG, "stopping net_handler");
	stop_udp_conn();
	if (robot_desc_read(SHEPHERD) == CONNECTED) {
		stop_tcp_conn(SHEPHERD);
	}
	if (robot_desc_read(DAWN) == CONNECTED) {
		stop_tcp_conn(DAWN);
	}
	shm_aux_stop(NET_HANDLER);
	shm_stop(NET_HANDLER);
	logger_stop(NET_HANDLER);
	//sockfd is automatically closed when process terminates
	exit(0);
}

// ******************************************* MAIN ROUTINE ******************************* //

int main ()
{
	int sockfd = -1, connfd = -1;
	struct sockaddr_in cli_addr; //requesting client's address
	socklen_t cli_addr_len = sizeof(struct sockaddr_in); //length of requesting client's address in bytes
	uint8_t client_id; //this is 0 if shepherd, 1 if dawn
	
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

	//start UDP connection with Dawn
	start_udp_conn(); 
	
	//run net_handler main control loop
	while (1) {
		//wait for a client to make a request to the robot, and accept it
		if ((connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_addr_len)) < 0) {
			perror("accept");
			log_printf(ERROR, "listen socket failed to accept a connection");
			continue;
		}
		cli_addr_len = sizeof(struct sockaddr_in);
		log_printf(DEBUG, "Received connection request from %s:%d", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
		
		//get the client ID (first byte on the socket from client)
		if (read(connfd, &client_id, 1) == -1) {
			log_printf(ERROR, "Couldn't get client type: %s", strerror(errno));
			continue;
		}
		
		//if the incoming request is shepherd or dawn, start the appropriate threads
		if (client_id == 0 && robot_desc_read(SHEPHERD) == DISCONNECTED) {
			log_printf(DEBUG, "Starting Shepherd connection");
			start_tcp_conn(SHEPHERD, connfd, 0);
		} else if (client_id == 1 && robot_desc_read(DAWN) == DISCONNECTED) {
			log_printf(DEBUG, "Starting Dawn connection");
			start_tcp_conn(DAWN, connfd, 1);
		} else {
			log_printf(ERROR, "Client is neither Dawn nor Shepherd");
			close(connfd);
		}
	}
	
	return 0;
}
