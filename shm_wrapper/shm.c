#include <signal.h>
#include "shm_wrapper.h"

//MAKE SURE YOU COPY-PASTE THIS FROM SHM_WRAPPER.C WHENEVER CHANGES ARE MADE TO SHM_WRAPPER.C!
//below we re-define all of the global variables, private typedefs, and defined constants in shm_wrapper.c
//this is preferred over moving all of these to shm_wrapper.h because doing that would
//cause some major namespace pollution (no other external process needs to know about these)

//names of various objects used in shm_wrapper; should not be used outside of shm_wrapper and shm_process
#define CATALOG_MUTEX_NAME "/ct-mutex"  //name of semaphore used as a mutex on the catalog
#define CMDMAP_MUTEX_NAME "/cmap-mutex" //name of semaphore used as a mutex on the command bitmap
#define SUBMAP_MUTEX_NAME "/smap-mutex" //name of semaphore used as a mutex on the various subcription bitmaps
#define DEV_SHM_NAME "/dev-shm"         //name of shared memory block across devices

#define GPAD_SHM_NAME "/gp-shm"         //name of shared memory block for gamepad
#define ROBOT_DESC_SHM_NAME "/rd-shm"   //name of shared memory block for robot description
#define GP_MUTEX_NAME "/gp-sem"         //name of semaphore used as mutex over gamepad shm
#define RD_MUTEX_NAME "/rd-sem"         //name of semaphore used as mutex over robot description shm

#define SNAME_SIZE 32 //size of buffers that hold semaphore names, in bytes

// *********************************** PRIVATE TYPEDEFS  ************************************************** //

//shared memory has these parts in it
typedef struct dev_shm {
	uint32_t catalog;                                   //catalog of valid devices
	uint32_t cmd_map[MAX_DEVICES + 1];                  //bitmap is 33 32-bit integers (changed devices and changed params of device commands from executor to dev_handler)
	uint32_t net_sub_map[MAX_DEVICES + 1];              //bitmap is 33 32-bit integers (changed devices and changed params in which data net_handler is subscribed to)
	uint32_t exec_sub_map[MAX_DEVICES + 1];             //bitmap is 33 32-bit integers (changed devices and changed params in which data executor is subscribed to)
	param_val_t params[2][MAX_DEVICES][MAX_PARAMS];     //all the device parameter info, data and commands
	dev_id_t dev_ids[MAX_DEVICES];                      //all the device identification info
} dev_shm_t;

//two mutex semaphores for each device
typedef struct sems {
	sem_t *data_sem;        //semaphore on the data stream of a device
	sem_t *command_sem;     //semaphore on the command stream of a device
} dual_sem_t;

//shared memory for gamepad
typedef struct gp_shm {
	uint32_t buttons;       //bitmap for which buttons are pressed
	float joysticks[4];     //array to hold joystick positions
} gamepad_shm_t;

//shared memory for robot description
typedef struct robot_desc_shm {
	uint8_t fields[NUM_DESC_FIELDS];   //array to hold the robot state (each is a uint8_t) 
} robot_desc_shm_t;

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

//this is NOT the same as the function in shm_wrapper.c! This creates the semaphore with permissions
sem_t *my_sem_open (char *sem_name, char *sem_desc)
{
	sem_t *ret;
	if ((ret = sem_open(sem_name, O_CREAT, 0660, 1)) == SEM_FAILED) {
		log_printf(FATAL, "sem_open: %s. %s", sem_desc, strerror(errno));
		exit(1);
	}
	return ret;
}

void my_sem_close (sem_t *sem, char *sem_desc)
{
	if (sem_close(sem) == -1) {
		log_printf(ERROR, "sem_close: %s. %s", sem_desc, strerror(errno));
	}
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

//clean up all the semaphores and shared memory (all global variables are accessed through shm_wrapper.c)
void sigint_handler (int signum)
{
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
	
	//unlink all shared memory blocks
	my_shm_unlink(DEV_SHM_NAME, "dev_shm");
	my_shm_unlink(GPAD_SHM_NAME, "gamepad_shm");
	my_shm_unlink(ROBOT_DESC_SHM_NAME, "robot_desc_shm");
	
	//close all the semaphores
	my_sem_close(catalog_sem, "catalog sem");
	my_sem_close(cmd_map_sem, "cmd map sem");
	my_sem_close(sub_map_sem, "sub map sem");
	my_sem_close(gp_sem, "gamepad_mutex");
	my_sem_close(rd_sem, "robot_desc_mutex");
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
	
	logger_stop();
	exit(0);
}

int main ()
{
	//set up
	signal(SIGINT, sigint_handler);
	logger_init(SUPERVISOR);
	
	int fd_shm;
	
	//create all semaphores with initial value 1
	catalog_sem = my_sem_open(CATALOG_MUTEX_NAME, "catalog mutex");
	cmd_map_sem = my_sem_open(CMDMAP_MUTEX_NAME, "cmd map mutex");
	sub_map_sem = my_sem_open(SUBMAP_MUTEX_NAME, "sub map mutex");
	gp_sem = my_sem_open(GP_MUTEX_NAME, "gamepad mutex");
	rd_sem = my_sem_open(RD_MUTEX_NAME, "robot desc mutex");
	for (int i = 0; i < MAX_DEVICES; i++) {
		generate_sem_name(DATA, i, sname); //get the data name
		if ((sems[i].data_sem = sem_open((const char *) sname, O_CREAT, 0660, 1)) == SEM_FAILED) {
			log_printf(FATAL, "sem_open: data sem for dev_ix %d: %s", i, strerror(errno));
			exit(1);
		}
		generate_sem_name(COMMAND, i, sname); //get the command name
		if ((sems[i].command_sem = sem_open((const char *) sname, O_CREAT, 0660, 1)) == SEM_FAILED) {
			log_printf(FATAL, "sem_open: command sem for dev_ix %d: %s", i, strerror(errno));
			exit(1);
		}
	}
	
	//create device shm block
	if ((fd_shm = shm_open(DEV_SHM_NAME, O_RDWR | O_CREAT, 0660)) == -1) {
		log_printf(FATAL, "shm_open: %s", strerror(errno));
		exit(1);
	}
	if (ftruncate(fd_shm, sizeof(dev_shm_t)) == -1) {
		log_printf(FATAL, "ftruncate: %s", strerror(errno));
		exit(1);
	}
	if ((dev_shm_ptr = mmap(NULL, sizeof(dev_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
		log_printf(FATAL, "mmap: %s", strerror(errno));
		exit(1);
	}
	if (close(fd_shm) == -1) {
		log_printf(ERROR, "close: %s", strerror(errno));
	}
	
	//create gamepad shm block
	if ((fd_shm = shm_open(GPAD_SHM_NAME, O_RDWR | O_CREAT, 0660)) == -1) {
		log_printf(FATAL, "shm_open gamepad_shm: %s", strerror(errno));
		exit(1);
	}
	if (ftruncate(fd_shm, sizeof(gamepad_shm_t)) == -1) {
		log_printf(FATAL, "ftruncate gamepad_shm: %s", strerror(errno));
		exit(1);
	}
	if ((gp_shm_ptr = mmap(NULL, sizeof(gamepad_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
		log_printf(FATAL, "mmap gamepad_shm: %s", strerror(errno));
		exit(1);
	}
	if (close(fd_shm) == -1) {
		log_printf(ERROR, "close gamepad_shm: %s", strerror(errno));
	}
	
	//create robot description shm block
	if ((fd_shm = shm_open(ROBOT_DESC_SHM_NAME, O_RDWR | O_CREAT, 0660)) == -1) {
		log_printf(FATAL, "shm_open robot_desc_shm: %s", strerror(errno));
		exit(1);
	}
	if (ftruncate(fd_shm, sizeof(robot_desc_shm_t)) == -1) {
		log_printf(FATAL, "ftruncate robot_desc_shm: %s", strerror(errno));
		exit(1);
	}
	if ((rd_shm_ptr = mmap(NULL, sizeof(robot_desc_shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
		log_printf(FATAL, "mmap robot_desc_shm: %s", strerror(errno));
		exit(1);
	}
	if (close(fd_shm) == -1) {
		log_printf(ERROR, "close robot_desc_shm: %s", strerror(errno));
	}
	
	//initialize everything
	dev_shm_ptr->catalog = 0;
	for (int i = 0; i < MAX_DEVICES + 1; i++) {
		dev_shm_ptr->cmd_map[i] = 0;
		dev_shm_ptr->net_sub_map[i] = 0;
		dev_shm_ptr->exec_sub_map[i] = 0;
	}
	gp_shm_ptr->buttons = 0;
	for (int i = 0; i < 4; i++) {
		gp_shm_ptr->joysticks[i] = 0.0;
	}
	rd_shm_ptr->fields[RUN_MODE] = IDLE;
	rd_shm_ptr->fields[DAWN] = DISCONNECTED;
	rd_shm_ptr->fields[SHEPHERD] = DISCONNECTED;
	rd_shm_ptr->fields[GAMEPAD] = DISCONNECTED;
	rd_shm_ptr->fields[START_POS] = LEFT;
	
	//now we just stall and wait
	while (1) {
		sleep(60);
	}
	
	return 0; //never reached
}