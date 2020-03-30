#include <stdio.h>
#include <stdlib.h> //malloc, atoi, etc.
#include <string.h> //string manipulation
#include <errno.h> //error handling functions (useful for printing errors)
#include <unistd.h> //standard symobolic constants, utility functions (i.e. sleep)

#include <sys/shm.h> //shm functions and constants 
#include <sys/ipc.h> //for IPC_CREAT and other constants like that
#include <sys/types.h> //time_t, size_t, ssize_t, pthread types, ipc types

//heavy copying from https://www.tutorialspoint.com/inter_process_communication/inter_process_communication_shared_memory.htm

#define BUF_SIZE 1024
#define SHM_KEY 0x1234 //some number
#define LOOPS 5 //number of BUF_SIZE blocks to print

struct shm_seg {
	int cnt;
	int complete;
	char buf[BUF_SIZE];
};

int fill_buffer (char *buf, int size)
{
	static char ch = 'A'; //retains betweeen function calls
	
	memset(buf, ch++, size - 1); //set the first (size - 1) chars of the buffer to "ch" (i.e. put 1023 ch's into the string)
	buf[size - 1] = '\0'; //null-terminate it
	
	if (ch > 122) { //larger than last letter of alphabet in ASCII ('z')
		ch = 65; //set it back to 'A'
	}
	
	return strlen(buf); //return number of filled chars
}

int main()
{
	int shm_id;
	struct shm_seg *shm;
	char *buf;
	
	shm_id = shmget(SHM_KEY, sizeof(struct shm_seg), 0666 | IPC_CREAT); //everybody can read/write to this piece of shared memory
	printf("shm_id = %d\n", shm_id);
	if (shm_id == -1) {
		perror("shared memory");
		return 1;
	}
	
	//attach the segment and get pointer to it
	shm = shmat(shm_id, NULL, 0); //NULL means give me a suitable address, 0 means no special flags on this segment
	if (shm == (void *) -1) { //need the case bc shm is a pointer
		perror("attaching");
		return 2;
	}
	
	//actually write to the block
	buf = shm->buf;
	for (int loops = 0; loops < LOOPS; loops++) {
		shm->cnt = fill_buffer(buf, BUF_SIZE);
		shm->complete = 0;
		printf("Writing process: wrote %d bytes\n", shm->cnt);
		sleep(3);
	}
	printf("Writing process: wrote %d times\n", LOOPS);
	shm->complete = 1;
	
	//detach the segment
	if (shmdt(shm) == -1) {
		perror("detaching");
		return 3;
	}
	
	//mark it to be destroyed after all process detach from it
	if (shmctl(shm_id, IPC_RMID, 0) == -1) {
		perror("removal");
		return 4;
	}
	
	printf("Writing process: complete!\n");
	
	return 0;
}