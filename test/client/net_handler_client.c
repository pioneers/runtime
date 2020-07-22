#include "net_handler_client.h"

//throughout this code, net_handler is abbreviated "nh"
pid_t nh_pid;          //holds the pid of the net_handler process
pthread_t dump_tid;    //holds the thread id of the output dumper thread

int nh_stdout_fd = -1;      //holds file descriptor of net handler process stdin
int nh_tcp_shep_fd = -1;    //holds file descriptor for TCP Shepherd socket
int nh_tcp_dawn_fd = -1;    //holds file descriptor for TCP Dawn socket
int nh_udp_fd = -1;         //holds file descriptor for UDP Dawn socket

// ************************************* FUNCITON DEFINITONS ************************** //

//dumps output from net handler stdout to this process's standard out
static void *output_dump (void *args)
{
	char buf[MAX_LOG_LEN];
	FILE *fd;
	if ((fd = fdopen(nh_stdout_fd, "r")) == NULL) {
		printf("fdopen: could not read stdout as FILE *: %s\n", strerror(errno));
	}
	
	while (1) {
		//if fgets errors out, it's because the thread was canceled
		if (fgets(buf, MAX_LOG_LEN, fd) == NULL) {
			return NULL;
		}
		printf("%s", buf);
	}
	return NULL;
}

static int connect_tcp (uint8_t client)
{
	struct sockaddr_in serv_addr, cli_addr;
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("socket: failed to create listening socket: %s\n", strerror(errno));
        stop_net_handler();
		exit(1);
	}

	int optval = 1;
	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int))) != 0) {
		printf("setsockopt: failed to set listening socket for reuse of port: %s\n", strerror(errno));
	}
	
	//set the elements of cli_addr
	memset(&cli_addr, '\0', sizeof(struct sockaddr_in));     //initialize everything to 0
	cli_addr.sin_family = AF_INET;                           //use IPv4
	cli_addr.sin_port = (client == SHEPHERD_CLIENT) ? htons(SHEPHERD_PORT) : htons(DAWN_PORT); //use specifid port to connect
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);            //use any address set on this machine to connect
	
	//bind the client side too, so that net_handler can verify it's the proper client
	if ((bind(sockfd, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr_in))) != 0) {
		printf("bind: failed to bind listening socket to client port: %s\n", strerror(errno));
		close(sockfd);
        stop_net_handler();
		exit(1);
	}
	
	//set the elements of serv_addr
	memset(&serv_addr, '\0', sizeof(struct sockaddr_in));     //initialize everything to 0
	serv_addr.sin_family = AF_INET;                           //use IPv4
	serv_addr.sin_port = htons(RASPI_PORT);                   //want to connect to raspi port
	serv_addr.sin_addr.s_addr = inet_addr(RASPI_ADDR);
	
	
	//connect to the server
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("connect: failed to connect to socket: %s\n", strerror(errno));
		close(sockfd);
        stop_net_handler();
		exit(1);
	}
	
	//send the verification byte
	writen(sockfd, &client, 1);
	
	return sockfd;
}

void usage ()
{
	printf("Usage: ./net_handler_cli TEST to run in test mode (runs in CLI mode by default)\n");
}

void start_net_handler (struct sockaddr_in *udp_servaddr)
{
	int fd[2]; //for holding file descriptor pair from pipe
	
	//create the pipe to capture net_handler's output
	if (pipe(fd) < 0) {
		printf("pipe: %s\n", strerror(errno));
	}
	
	//fork net_handler process
	if ((nh_pid = fork()) < 0) {
		printf("fork: %s\n", strerror(errno));
	} else if (nh_pid > 0) { //parent
		close(fd[1]); //close write end
		nh_stdout_fd = fd[0];
	} else { //child
		close(fd[0]); //close read end
		//attaches the write end of the pipe to standard out
		if (fd[1] != STDOUT_FILENO) {
			if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO) {
				printf("dup2 stdin: %s\n", strerror(errno));
			}
			close(fd[1]); //don't need this after dup2
		}
		if (execlp("../../net_handler/net_handler", "net_handler", (char *) 0) < 0) {
			printf("execlp: %s\n", strerror(errno));
		}
	}
	
	sleep(1); //allows net_handler to set itself up
	
	//Connect to the raspi networking ports to catch network output
	nh_tcp_shep_fd = connect_tcp(SHEPHERD_CLIENT);
	nh_tcp_dawn_fd = connect_tcp(DAWN_CLIENT);
	
    if ((nh_udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        printf("socket creation failed...\n"); 
        stop_net_handler();
		exit(1);
    }
	//set the UDP server address
    memset(udp_servaddr, 0, sizeof(struct sockaddr_in));
    udp_servaddr->sin_family = AF_INET; 
    udp_servaddr->sin_addr.s_addr = inet_addr(RASPI_ADDR); 
    udp_servaddr->sin_port = htons(RASPI_UDP_PORT);
	
	//start the thread that is dumping output from net_handler to stdout of this process
	if (pthread_create(&dump_tid, NULL, output_dump, NULL) != 0) {
		printf("pthread_create: output dump\n");
	}
}

void stop_net_handler ()
{
	//send signal to net_handler and wait for termination
	if (kill(nh_pid, SIGINT) < 0) {
		printf("kill: %s\n", strerror(errno));
	}
	if (waitpid(nh_pid, NULL, 0) < 0) {
		printf("waitpid: %s\n", strerror(errno));
	}
	
	//close all the file descriptors
	if (nh_stdout_fd == -1) {
		close(nh_stdout_fd);
	}
	if (nh_tcp_shep_fd == -1) {
		close(nh_tcp_shep_fd);
	}
	if (nh_tcp_dawn_fd == -1) {
		close(nh_tcp_dawn_fd);
	}
	if (nh_udp_fd == -1) {
		close(nh_udp_fd);
	}
	
	//stop the output dump thread
	if (pthread_cancel(dump_tid) != 0) {
		printf("pthread_cancel: output dump\n");
	}
	if (pthread_join(dump_tid, NULL) != 0) {
		printf("pthread_join: output dump\n");
	}
}
