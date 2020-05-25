/* good demonstration of how to use POSIX shm with semaphores
 * from https://www.softprayog.in/programming/interprocess-communication-using-posix-shared-memory-in-linux
 */

#include <stdio.h>			//for i/o
#include <stdlib.h>			//for standard utility functions (exit, sleep)
#include <sys/types.h>		//for sem_t and other standard system types
#include <sys/stat.h>		//for some of the flags that are used (the mode constants)
#include <fcntl.h>			//for flags used for opening and closing files (O_* constants)
#include <string.h>			//for string operations
#include <time.h>			//for printing system time
#include <unistd.h>			//for standard symbolic constants
#include <semaphore.h>		//for semaphores
#include <sys/mman.h>		//for posix shared memory

/*
Useful function definitions:

int shm_open (const char *name, int oflag, mode_t mode); //if O_CREAT is not specified or the shm object exists, mode is ignored
int shm_unlink (const char *name);
sem_t *sem_open (const char *name, int oflag, mode_t mode, unsigned int value);
int ftruncate (int fd, off_t length);
void *mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap (void *addr, size_t length);
*/

#define MAX_BUFFERS 10

#define LOGFILE "/Users/ben/Desktop/example.log"

#define SEM_MUTEX_NAME "/sem-mutex"
#define SEM_BUFFER_COUNT_NAME "/sem-buffer-count"
#define SEM_SPOOL_SIGNAL_NAME "/sem-spool-signal"
#define SHARED_MEM_NAME "/posix-shared-mem-example"

struct shared_memory {
	char buf[MAX_BUFFERS][256];	//the actual buffer
	int buffer_index;				//index that we're currently at
	int buffer_print_index;			//everything up to here has been printed
};

void error (char *msg);

int main (int argc, char **arg)
{
	struct shared_memory *shared_mem_ptr;
	sem_t *mutex_sem, *buffer_count_sem, *spool_signal_sem;
	int fd_shm;
	
	//mutual exclusion semaphore, mutex_sem. sem will be created already, so just specify name, then 0 for oflag, mode, and value
	if ((mutex_sem = sem_open(SEM_MUTEX_NAME, 0, 0, 0)) == SEM_FAILED) {
		error("sem_open");
	}
	
	//get shared memory. first "open the file" and then map into virtual memory
	if ((fd_shm = shm_open(SHARED_MEM_NAME, O_RDWR, 0)) == -1) {
		error("shm_open");
	}
	if ((shared_mem_ptr = mmap(NULL, sizeof(struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
		error("mmap");
	}
	
	//counting semaphore, indicating # of available buffers
	if ((buffer_count_sem = sem_open(SEM_BUFFER_COUNT_NAME, 0, 0, 0)) == SEM_FAILED) {
		error("sem_open");
	}
	
	//counting semaphore, indicating # of strings to be printed; initial value = 0
	if ((spool_signal_sem = sem_open(SEM_SPOOL_SIGNAL_NAME, 0, 0, 0)) == SEM_FAILED) {
		error("sem_open");
	}
	
	char buf[200], *cp;
	
	printf("Please type a mesage: ");
	
	//print MAX_BUFFERS times and then stop
	for (int i = 0; i < MAX_BUFFERS; i++) {
		fgets(buf, 198, stdin);
		
		//remove newline from string
		int length = strlen(buf);
		if (buf[length - 1] == '\n') {
			buf[length - 1] = '\0';
		}
		
		//get a buffer
		if (sem_wait(buffer_count_sem) == -1) {
			error("sem_wait: buffer_count_sem");
		}
		
		//if multiple producers, only one producer can use buffer_index at a time
		if (sem_wait(mutex_sem) == -1) {
			error("sem_wait: mutex_sem");
		}
		
		//critical section:
		time_t now = time(NULL); //get the time formatted in local time
		cp = ctime(&now);
		int len = strlen(cp);
		if (*(cp + len - 1) == '\n') {
			*(cp + len - 1) = '\0';
		}
		sprintf(shared_mem_ptr->buf[shared_mem_ptr->buffer_index], "%d: %s %s\n", getpid(), cp, buf); //assemble and print into the shm buffer
		(shared_mem_ptr->buffer_index)++;
		if (shared_mem_ptr->buffer_index == MAX_BUFFERS) {
			shared_mem_ptr->buffer_index = 0;
		}
		
		//release mutex sem
		if (sem_post(mutex_sem) == -1) {
			error("sem_post: mutex_sem");
		}
		
		//tell spooler that there is a string to print
		if (sem_post(spool_signal_sem) == -1) {
			error("sem_post: (spool_signal_sem)");
		}
		
		printf("Please type a message: ");
	}
	
	//unamp the shared memory
	if (munmap(shared_mem_ptr, sizeof(struct shared_memory)) == -1) {
		error("munmap");
	}
	
	//close the file descriptor
	if (close(fd_shm) == -1) {
		error("close");
	}
	
	//close the semaphores (no unlink! this is done in the other process)
	if (sem_close(mutex_sem) == -1) {
		error("sem_close: mutex_sem");
	}
	if (sem_close(spool_signal_sem) == -1) {
		error("sem_close: spool_signal_sem");
	}
	if (sem_close(buffer_count_sem) == -1) {
		error("sem_close: buffer_count_sem");
	}
	
	return 0;
}

//print system error and exit
void error (char *msg)
{
	perror(msg);
	exit(1);
}
