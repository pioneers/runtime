#include <signal.h>
#include "shm_wrapper.h"

// *********************************** WRAPPER-SPECIFIC GLOBAL VARS **************************************** //

dual_sem_t sems[MAX_DEVICES];  //array of semaphores, two for each possible device (one for data and one for commands)
dev_shm_t *dev_shm_ptr;        //points to memory-mapped shared memory block for device data and commands
sem_t *catalog_sem;            //semaphore used as a mutex on the catalog
sem_t *cmd_map_sem;            //semaphore used as a mutex on the command bitmap
sem_t *sub_map_sem;            //semaphore used as a mutex on the subscription bitmap

gamepad_shm_t *gp_shm_ptr;     //points to memory-mapped shared memory block for gamepad
robot_desc_shm_t *rd_shm_ptr;  //points to memory-mapped shared memory block for robot description
sem_t *gp_sem;                 //semaphore used as a mutex on the gamepad
sem_t *rd_sem;                 //semaphore used as a mutex on the robot description

log_data_shm_t *log_data_shm_ptr;  // points to shared memory block for log data specified by executor
sem_t *log_data_sem;			   //semaphore used as a mutex on the log data

// *********************************** SHM PROCESS FUNCTIONS ************************************************* //

char sname[SNAME_SIZE];   //being lazy here but this is for holding all the semaphore names

void generate_sem_name (stream_t stream, int dev_ix, char *name)
{
	if (stream == DATA) {
		sprintf(name, "/data_sem_%d", dev_ix);
	} else if (stream == COMMAND) {
		sprintf(name, "/command_sem_%d", dev_ix);
	}
}

void my_sem_close (sem_t *sem, char *sem_desc)
{
	if (sem_close(sem) == -1) {
		log_printf(ERROR, "sem_close: %s. %s", sem_desc, strerror(errno));
	}
}

// this is the my_sem_open in shm_wrapper.c, not the my_sem_open in shm_start.c
sem_t *my_sem_open (char *sem_name, char *sem_desc)
{
	sem_t *ret;
	if ((ret = sem_open(sem_name, 0, 0, 0)) == SEM_FAILED) {
		log_printf(FATAL, "sem_open: %s. %s", sem_desc, strerror(errno));
		exit(1);
	}
	return ret;
}

void my_shm_unlink (char *shm_name, char *shm_desc)
{
	if (shm_unlink(shm_name) == -1) {
		log_printf(ERROR, "shm_unlink: %s. %s", shm_desc, strerror(errno));
	}
}

void my_sem_unlink (char *sem_name, char *sem_desc)
{
	if (sem_unlink(sem_name) == -1) {
		log_printf(ERROR, "sem_unlink: %s. %s", sem_desc, strerror(errno));
	}
}

int main ()
{
    //init the logger
    logger_init(SHM);
    int fd_shm;
    
    // ************************************* OPEN EVERYTHING ******************************* //
    
	//open all the semaphores
	catalog_sem = my_sem_open(CATALOG_MUTEX_NAME, "catalog mutex");
	cmd_map_sem = my_sem_open(CMDMAP_MUTEX_NAME, "cmd map mutex");
	sub_map_sem = my_sem_open(SUBMAP_MUTEX_NAME, "sub map mutex");
	gp_sem = my_sem_open(GP_MUTEX_NAME, "gamepad mutex");
	rd_sem = my_sem_open(RD_MUTEX_NAME, "robot desc mutex");
	log_data_sem = my_sem_open(LOG_DATA_MUTEX, "log data mutex");
	for (int i = 0; i < MAX_DEVICES; i++) {
		generate_sem_name(DATA, i, sname); //get the data name
		if ((sems[i].data_sem = sem_open((const char *) sname, 0, 0, 0)) == SEM_FAILED) {
			log_printf(ERROR, "sem_open: data sem for dev_ix %d: %s", i, strerror(errno));
		}
		generate_sem_name(COMMAND, i, sname); //get the command name
		if ((sems[i].command_sem = sem_open((const char *) sname, 0, 0, 0)) == SEM_FAILED) {
			log_printf(ERROR, "sem_open: command sem for dev_ix %d: %s", i, strerror(errno));
		}
	}
    
	//open dev shm block and map to client process virtual memory
	if ((fd_shm = shm_open(DEV_SHM_NAME, O_RDWR, 0)) == -1) { //no O_CREAT
		log_printf(FATAL, "shm_open dev_shm %s", strerror(errno));
		exit(1);
	}
	if ((dev_shm_ptr = mmap(NULL, sizeof(dev_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
		log_printf(FATAL, "mmap dev_shm: %s", strerror(errno));
		exit(1);
	}
	if (close(fd_shm) == -1) {
		log_printf(ERROR, "close dev_shm %s", strerror(errno));
	}
    
	//open gamepad shm block and map to client process virtual memory
	if ((fd_shm = shm_open(GPAD_SHM_NAME, O_RDWR, 0)) == -1) { //no O_CREAT
		log_printf(FATAL, "shm_open: gamepad_shm. %s", strerror(errno));
		exit(1);
	}
	if ((gp_shm_ptr = mmap(NULL, sizeof(gamepad_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
		log_printf(FATAL, "mmap: gamepad_shm. %s", strerror(errno));
		exit(1);
	}
	if (close(fd_shm) == -1) {
		log_printf(ERROR, "close: gamepad_shm. %s", strerror(errno));
	}
    
	//open robot desc shm block and map to client process virtual memory
	if ((fd_shm = shm_open(ROBOT_DESC_SHM_NAME, O_RDWR, 0)) == -1) { //no O_CREAT
		log_printf(FATAL, "shm_open: robot_desc_shm. %s", strerror(errno));
		exit(1);
	}
	if ((rd_shm_ptr = mmap(NULL, sizeof(robot_desc_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
		log_printf(FATAL, "mmap: robot_desc_shm. %s", strerror(errno));
		exit(1);
	}
	if (close(fd_shm) == -1) {
		log_printf(ERROR, "close: robot_desc_shm. %s", strerror(errno));
	}
    
	//create log data shm block
	if ((fd_shm = shm_open(LOG_DATA_SHM, O_RDWR | O_CREAT, 0660)) == -1) {
		log_printf(FATAL, "shm_open log_data_shm: %s", strerror(errno));
		exit(1);
	}
	if ((log_data_shm_ptr = mmap(NULL, sizeof(log_data_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
		log_printf(FATAL, "mmap log_data_shm: %s", strerror(errno));
		exit(1);
	}
	if (close(fd_shm) == -1) {
		log_printf(ERROR, "close log_data_shm: %s", strerror(errno));
	}
    
     // ******************************* CLOSE + UNLINK EVERYTHING ************************** //
    
	//unmap all shared memory blocks
	if (munmap(dev_shm_ptr, sizeof(dev_shm_t)) == -1) {
		log_printf(ERROR, "munmap: dev_shm_ptr. %s", strerror(errno));
	}
	if (munmap(gp_shm_ptr, sizeof(gamepad_shm_t)) == -1) {
		log_printf(ERROR, "munmap: gamepad_shm_ptr. %s", strerror(errno));
	}
	if (munmap(rd_shm_ptr, sizeof(robot_desc_shm_t)) == -1) {
		log_printf(ERROR, "munmap: robot_desc_shm_ptr. %s", strerror(errno));
	}
	if (munmap(log_data_shm_ptr, sizeof(log_data_shm_t)) == -1) {
		log_printf(ERROR, "munmap: log_data_shm_ptr. %s", strerror(errno));
	}
	
	//unlink all shared memory blocks
	my_shm_unlink(DEV_SHM_NAME, "dev_shm");
	my_shm_unlink(GPAD_SHM_NAME, "gamepad_shm");
	my_shm_unlink(ROBOT_DESC_SHM_NAME, "robot_desc_shm");
	my_shm_unlink(LOG_DATA_SHM, "log_data_shm");
	
	//close all the semaphores
	my_sem_close(catalog_sem, "catalog sem");
	my_sem_close(cmd_map_sem, "cmd map sem");
	my_sem_close(sub_map_sem, "sub map sem");
	my_sem_close(gp_sem, "gamepad_mutex");
	my_sem_close(rd_sem, "robot_desc_mutex");
	my_sem_close(log_data_sem, "log data mutex");
	for (int i = 0; i < MAX_DEVICES; i++) {
		my_sem_close(sems[i].data_sem, "data sem");
		my_sem_close(sems[i].command_sem, "command sem");
	}
	
	//unlink all semaphores
	my_sem_unlink(CATALOG_MUTEX_NAME, "catalog mutex");
	my_sem_unlink(CMDMAP_MUTEX_NAME, "cmd map mutex");
	my_sem_unlink(SUBMAP_MUTEX_NAME, "sub map mutex");
	my_sem_unlink(GP_MUTEX_NAME, "gamepad mutex");
	my_sem_unlink(RD_MUTEX_NAME, "robot desc mutex");
	my_sem_unlink(LOG_DATA_MUTEX, "log data mutex");
	for (int i = 0; i < MAX_DEVICES; i++) {
		generate_sem_name(DATA, i, sname);
		if (sem_unlink((const char *) sname) == -1) {
			log_printf(ERROR, "sem_unlink: data_sem for dev_ix %d. %s", i, strerror(errno));
		}
		generate_sem_name(COMMAND, i, sname);
		if (sem_unlink((const char *) sname) == -1) {
			log_printf(ERROR, "sem_unlink: command_sem for dev_ix %d. %s", i, strerror(errno));
		}
	}
	
	return 1; //returns with 1 to start up 
}