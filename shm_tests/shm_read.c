#include <stdio.h>
#include <stdlib.h> //malloc, atoi, etc.
#include <string.h> //string manipulation
#include <errno.h> //error handling functions (useful for printing errors)
#include <unistd.h> //standard symobolic constants, utility functions (i.e. sleep)

#include <sys/shm.h> //shm functions and constants 
#include <sys/ipc.h> //for IPC_CREAT and other constants like that
#include <sys/types.h> //time_t, size_t, ssize_t, pthread types, ipc types

#define BUF_SIZE 1024
#define SHM_KEY 0x1234 //some number
#define LOOPS 5 //number of BUF_SIZE blocks to print

struct shm_seg {
	int cnt;
	int complete;
	char buf[BUF_SIZE];
};

int main()
{
	int shm_id;
	struct shm_seg *shm;
	
	shm_id = shmget(SHM_KEY, sizeof(struct shm_seg), 0666 | IPC_CREAT);
	if (shm_id == -1) {
		perror("shared memory");
		return 1;
	}
	
	//attach to the shared memory and get pointer
	shm = shmat(shm_id, NULL, 0); //same as in write function
	if (shm == (void *) - 1) {
		perror("attaching");
		return 2;
	}
	
	//print out data in shared memory
	while (shm->complete != 1) {
		printf("segment contains: \n \" %s \" \n", shm->buf);
		if (shm->cnt == -1) {
			perror("reading error caused by write");
			return 3;
		}
		printf("Reading process: read %d bytes\n", shm->cnt);
		sleep(3);
	}
	
	//detach the shared memory (will be set to destroy from other process)
	if (shmdt(shm) == -1) {
		perror("detaching");
		return 4;
	}
	
	printf("Reading process: done\n");
	
	return 0;
}