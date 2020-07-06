#include <stdio.h>       //for i/o
#include <stdlib.h>      //for exit
#include <string.h>      //for memset
#include <arpa/inet.h>   //for inet_addr, bind, listen, accept, socket types
#include <unistd.h>      //for read, write, close

#include "../net_util.h"

float data[4] = {.012, 13.1, -.13, .30};

int main() {
    logger_init(TEST);
    //Open UDP connection
    int sockfd; 
    struct sockaddr_in servaddr; 

    // socket create and varification 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        return 1;
    } else {
        printf("Socket successfully created..\n"); 
    }
    memset(&servaddr, 0, sizeof(servaddr));

    // server IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(DAWN_ADDR); 
    servaddr.sin_port = htons(UDP_PORT); 

    GpState gp_state = GP_STATE__INIT;
    gp_state.connected = 1;
    gp_state.buttons = 13879811;
    gp_state.n_axes = 4;
    gp_state.axes = &data[0];


    int len = gp_state__get_packed_size(&gp_state);
	
	uint8_t* buf = malloc(len);
	gp_state__pack(&gp_state, buf);

    log_printf(DEBUG, "About to send");    
    int err = sendto(sockfd, buf, len, 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
    log_printf(DEBUG, "sendto stopped blocking");
    if (err < 0) {
        perror("sendto");
        log_printf(DEBUG, "sendto failed");
        return 1;
    }
    free(buf);
    log_printf(DEBUG, "Sent data to socket");

    int max_size = 4096;
    uint8_t msg[max_size];
    struct sockaddr_in recvaddr;
    socklen_t addrlen = sizeof(recvaddr);
    int recv_size;
    if ((recv_size = recvfrom(sockfd, msg, max_size, 0, (struct sockaddr*) &recvaddr, &addrlen)) < 0) {
        perror("recvfrom");
        log_printf(DEBUG, "recvfrom failed");
    }
    log_printf(DEBUG, "Raspi IP is %s", inet_ntoa(recvaddr.sin_addr));
    log_printf(DEBUG, "received data size %d", recv_size);
    DevData* dev_data = dev_data__unpack(NULL, recv_size, msg);
    if (dev_data == NULL)
	{
		log_printf(ERROR, "error unpacking incoming message\n");
		return 1;
	}
    // display the message's fields.
	printf("Received:\n");
	for (int i = 0; i < dev_data->n_devices; i++) {
		printf("Device No. %d: ", i);
		printf("\ttype = %s, uid = %llu, itype = %d\n", dev_data->devices[i]->name, dev_data->devices[i]->uid, dev_data->devices[i]->type);
		printf("\tParams:\n");
		for (int j = 0; j < dev_data->devices[i]->n_params; j++) {
			printf("\t\tparam \"%s\" has type ", dev_data->devices[i]->params[j]->name);
			switch (dev_data->devices[i]->params[j]->val_case) {
				case (PARAM__VAL_FVAL):
					printf("FLOAT with value %f\n", dev_data->devices[i]->params[j]->fval);
					break;
				case (PARAM__VAL_IVAL):
					printf("INT with value %d\n", dev_data->devices[i]->params[j]->ival);
					break;
				case (PARAM__VAL_BVAL):
					printf("BOOL with value %d\n", dev_data->devices[i]->params[j]->bval);
					break;
				default:
					printf("ERROR: no param value");
					break;
			}
		}
	}

	// Free the unpacked message
	dev_data__free_unpacked(dev_data, NULL);
    fflush(stdout);
    return 0;
}
