#include "../client/net_handler_client.h"

#include <stdio.h>
#include <arpa/inet.h>  // for inet_addr, bind, listen, accept, socket types
#include <netinet/in.h> // for structures relating to IPv4 addresses
#include <netdb.h>      // for struct addrinfo
#define SA struct sockaddr 
#define LOCALHOST "192.168.65.2"
#define PORT 5006 // This port must be exposed in docker-compose.yml


char *file = "/cli/gamepad_inputs.fifo";
int fd;
FILE *fp;
char character;
u_int32_t buttons = 0;
float joystick_vals[4];
void set_state(int set);
char buff[33];

// ********************************** MAIN PROCESS ****************************************** //
void sigint_handler(int signum) {
    close_output();
    stop_net_handler();
    exit(0);
}
void set_state(int set){
    buttons |= (1 << set);
}


int connect_tcp(){
    int sockfd, connfd;
    struct sockaddr_in servaddr = {0}; 
    struct sockaddr_in cli = {0};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    printf("Socket creation successful\n");

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(5006);

    if((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0){
        printf("socket bind failed... %s\n", strerror(errno));
        exit(0);
    }
    printf("socket binded successfully...\n");

    if((listen(sockfd, 5)) != 0){
        printf("listen failed...\n");
        exit(0);
    }
    printf("Got client\n");
    socklen_t len = sizeof(cli);
    connfd = accept(sockfd, (struct sockaddr*)&cli, &len);
    printf("Finished accept function \n");
    if(connfd < 0){
        printf("server accept failed...\n");
        exit(0);
    }
    printf("server accept the client...\n");
    return connfd;
}

int main() {

    signal(SIGINT, sigint_handler);
    fd = connect_tcp();

    printf("Its gamer time\n");
    
    printf("Starting Net Handler CLI...\n");
    fflush(stdout);

    start_net_handler();

    while(1){
        buttons = 0;
        memset(buff, 0, 32 * sizeof(char));
        memset(joystick_vals, 0, 4 * sizeof(float));
        recv(fd, buff, sizeof(buff), 0);
        buff[32] = '\0';
        
        for(int i = 13; i < 21; i++){
            int pushed = 0;
            if (buff[i] == '1'){
                pushed = 1.0;
            }
            if(pushed != 0){
                switch(i){
                    
                    case 13:
                        joystick_vals[0] = pushed;
                        break;
                    case 14:
                        joystick_vals[0] = -1 * pushed;
                        break;
                    case 15:
                        joystick_vals[1] = -1 * pushed;
                        break;
                    case 16:
                        joystick_vals[1] = 1 * pushed;
                        break;
                    case 17:
                        joystick_vals[2] = -1 * pushed;
                        break;
                    case 18:
                        joystick_vals[2] = 1 * pushed;
                        break;
                    case 19:
                        joystick_vals[3] = -1 * pushed;
                        break;
                    case 20:
                        joystick_vals[3] = pushed;
                        break;
                }
            }
        } 

        for(int i = 0; i < 13; i++){
            if(buff[i] == '1'){
                buttons |= (1 << i);
            }
        }
    
        send_gamepad_state(buttons, joystick_vals);
        
    } 
    return 0;
}
