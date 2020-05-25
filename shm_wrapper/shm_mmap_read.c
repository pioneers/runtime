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

int shm_open (const char *name, int oflag, mode_t mode);
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
	int fd_shm, fd_log;
	char mybuf[256];
	
	//open log file 
	//(open and fopen since it's a toy example and we don't care about speed; https://stackoverflow.com/questions/1658476/c-fopen-vs-open for more info)
	if ((fd_log = open(LOGFILE, O_CREAT | O_WRONLY | O_APPEND | O_SYNC, 0666)) == -1) {
		error("fopen");
	}
	
	//mutual exclusion semaphore, mutex_sem with initial value = 0
	if ((mutex_sem = sem_open(SEM_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED) {
		error("sem_open");
	}
	
	//get shared memory
	if ((fd_shm = shm_open(SHARED_MEM_NAME, O_RDWR | O_CREAT, 0660)) == -1) {
		error("shm_open");
	}
	if (ftruncate(fd_shm, sizeof(struct shared_memory)) == -1) {
		error("ftruncate");
	}
	if ((shared_mem_ptr = mmap(NULL, sizeof(struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
		error("mmap");
	}
	shared_mem_ptr->buffer_index = shared_mem_ptr->buffer_print_index = 0; //initialize it
	
	//counting semaphore, indicating # of available buffers; initial value = MAX_BUFFERS
	if ((buffer_count_sem = sem_open(SEM_BUFFER_COUNT_NAME, O_CREAT, 0660, MAX_BUFFERS)) == SEM_FAILED) {
		error("sem_open");
	}
	
	//counting seamaphore, indicating # of strings to be printed; initial value = 0
	if ((spool_signal_sem = sem_open(SEM_SPOOL_SIGNAL_NAME, O_CREAT, 0660, 0)) == SEM_FAILED) {
		error("sem_open");
	}
	
	//initialization complete; set mutex semaphore to 1 indicating shm segment available for client(s)
	if (sem_post(mutex_sem) == -1) {
		error("sem_post: mutex_sem");
	}
	
	//print MAX_BUFFERS messages and then done
	for (int i = 0; i < MAX_BUFFERS; i++) {
		//is there a string to print?
		if (sem_wait(spool_signal_sem) == -1) {
			error("sem_wait: spool_signal_sem");
		}
		
		strcpy(mybuf, shared_mem_ptr->buf[shared_mem_ptr->buffer_print_index]);
		
		//since only this process using buffer_print_index, no need to wait on mutex_semaphore
		(shared_mem_ptr->buffer_print_index)++;
		if (shared_mem_ptr->buffer_print_index == MAX_BUFFERS) {
			shared_mem_ptr->buffer_print_index = 0;
		}
		
		//contents of one buffer printed; one more buffer available for producer(s); release buffer
		if (sem_post(buffer_count_sem) == -1) {
			error("sem_post: buffer_count_sem");
		}
		
		//write string to file
		if (write(fd_log, mybuf, strlen(mybuf)) != strlen(mybuf)) {
			error("write: logfile");
		}
	}
	
	//unamp the shared memory
	if (munmap(shared_mem_ptr, sizeof(struct shared_memory)) == -1) {
		error("munmap");
	}
	
	//close the file descriptor
	if (close(fd_shm) == -1) {
		error("close");
	}
	
	//unlink shared memory block
	if (shm_unlink(SHARED_MEM_NAME) == -1) {
		error("shm_unlink");
	}
	
	//close the semaphores
	if (sem_close(mutex_sem) == -1) {
		error("sem_close: mutex_sem");
	}
	if (sem_close(spool_signal_sem) == -1) {
		error("sem_close: spool_signal_sem");
	}
	if (sem_close(buffer_count_sem) == -1) {
		error("sem_close: buffer_count_sem");
	}
	
	//unlink the semaphores
	if (sem_unlink(SEM_MUTEX_NAME) == -1) {
		error("sem_unlink: mutex_sem");
	}
	if (sem_unlink(SEM_BUFFER_COUNT_NAME) == -1) {
		error("sem_unlink: buffer_count_sem");
	}
	if (sem_unlink(SEM_SPOOL_SIGNAL_NAME) == -1) {
		error("sem_unlink: spool_signal_sem");
	}
	
	return 0;
}

//print system error and exit
void error (char *msg)
{
	perror(msg);
	exit(1);
}
