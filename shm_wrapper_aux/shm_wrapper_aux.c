#include "shm_wrapper.h"

/*
Useful function definitions:

int shm_open (const char *name, int oflag, mode_t mode); //if O_CREAT is not specified or the shm object exists, mode is ignored
int shm_unlink (const char *name);
void *mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap (void *addr, size_t length);
int ftruncate (int fd, off_t length);

sem_t *sem_open (const char *name, int oflag, mode_t mode, unsigned int value); //if O_CREAT is not specified or the sem exists, mode is ignored
int sem_unlink (const char *name);
int sem_wait (sem_t *sem);
int sem_post (sem_t *sem);
*/

#define GPAD_SHM_NAME "/gp-shm"         //name of shared memory block for gamepad
#define ROBOT_DESC_SHM_NAME "/rd-shm"   //name of shared memory block for robot description
#define GP_MUTEX_NAME "/gp-sem"         //name of semaphore used as mutex over gamepad shm
#define RD_MUTEX_NAME "/rd-sem"         //name of semaphore used as mutex over robot description shm

#define NUM_DESC_FIELDS 6               //number of fields in the robot description
#define NUM_GAMEPAD_BUTTONS 17          //number of gamepad buttons

// ***************************************** PRIVATE TYPEDEFS ********************************************* //

//shared memory for gamepad
typedef struct gp {
	uint32_t buttons;                   //bitmap for which buttons are pressed
	float joysticks[4];                 //array to hold joystick positions
} gamepad_t;

//shared memory for robot description
typedef struct robot_desc {
	uint8_t fields[NUM_DESC_FIELDS];    //array to hold the robot state (each is a uint8_t)
} robot_desc_t;

// *********************************** WRAPPER-SPECIFIC GLOBAL VARS **************************************** //

gamepad_t *gp_ptr;                      //points to memory-mapped shared memory block for gamepad
robot_desc_t *rd_ptr;                   //points to memory-mapped shared memory block for robot description
sem_t *gp_sem;                          //semaphore used as a mutex on the gamepad
sem_t *rd_sem;                          //semaphore used as a mutex on the robot description

// ******************************************** HELPER FUNCTIONS ****************************************** //

static void error (char *msg)
{
	perror(msg);
	log_runtime(ERROR, msg); //send the message to the logger
	exit(1);
}

// ************************************ PUBLIC UTILITY FUNCTIONS ****************************************** //

void print_robot_desc ()
{
	//since there's no get_robot_desc function (we don't need it, and hides the implementation from users)
	//we need to acquire the semaphore for the print
	
	//wait on rd_sem
	if (sem_wait(rd_sem) == -1) {
		error("sem_wait: robot_desc_mutex (in print)");
	}
	
	printf("Current Robot Description:\n");
	for (int i = 0; i < NUM_DESC_FIELDS; i++) {
		switch (i) {
			case STATE:
				printf("\tSTATE = %s\n", (rd_ptr->fields[STATE] == ERROR) ? "ERROR" : "NOMINAL");
				break;
			case RUN_MODE:
				printf("\tRUN_MODE = %s\n", (rd_ptr->fields[RUN_MODE] == IDLE) ? "IDLE" : ((rd_ptr->fields[RUN_MODE] == AUTO) ? "AUTO" : "TELEOP"));
				break;
			case DAWN:
				printf("\tDAWN = %s\n", (rd_ptr->fields[DAWN] == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
				break;
			case SHEPHERD:
				printf("\tSHEPHERD = %s\n", (rd_ptr->fields[SHEPHERD] == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
				break;
			case GAMEPAD:
				printf("\tGAMEPAD = %s\n", (rd_ptr->fields[GAMEPAD] == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
				break;
			case TEAM:
				printf("\tTEAM = %s\n", (rd_ptr->fields[TEAM] == BLUE) ? "BLUE" : "GOLD");
				break;
			default:
				printf("unknown field\n");
		}
	}
	printf("\n");
	fflush(stdout);
	
	//release rd_sem
	if (sem_post(rd_sem) == -1) {
		error("sem_post: robot_desc_mutex (in print)");
	}
}

void print_gamepad_state ()
{
	//since there's no get_gamepad function (we don't need it, and hides the implementation from users)
	//we need to acquire the semaphore for the print
	
	//oof string arrays for printing
	char *button_names[NUM_GAMEPAD_BUTTONS] = {
		"A_BUTTON", "B_BUTTON", "X_BUTTON", "Y_BUTTON", "L_BUMPER", "R_BUMPER", "L_TRIGGER", "R_TRIGGER",
		"BACK_BUTTON", "START_BUTTON", "L_STICK", "R_STICK", "UP_DPAD", "DOWN_DPAD", "LEFT_DPAD", "RIGHT_DPAD", "XBOX_BUTTON"
	};
	char *joystick_names[4] = {
		"X_LEFT_JOYSTICK", "Y_LEFT_JOYSTICK", "X_RIGHT_JOYSTICK", "Y_RIGHT_JOYSTICK"
	};
	
	//wait on gp_sem
	if (sem_wait(gp_sem) == -1) {
		error("sem_wait: gamepad_mutex (in print)");
	}
	
	//only print pushed buttons (so we don't print out 22 lines of output each time we all this function)
	printf("Current Gamepad State:\n");
	for (int i = 0; i < NUM_GAMEPAD_BUTTONS; i++) {
		if (gp->buttons & (1 << i)) {
			printf("\t%s  ", button_names[i]);
		}
		printf("\n");
	}
	//print joystick positions
	for (int i = 0; i < 4; i++) {
		printf("\t %s = %f\n", joystick_names[i], gp->joysticks[i]);
	}
	printf("\n");
	fflush(stdout);
	
	//release rd_sem
	if (sem_post(gp_sem) == -1) {
		error("sem_post: gamepad_mutex (in print)");
	}
}

// ************************************ PUBLIC WRAPPER FUNCTIONS ****************************************** //

/*
Call this function from every process that wants to use the auxiliary shared memory wrapper
Should be called before any other action happens
The network handler process is responsible for initializing the shared memory blocks
	- process: one of DEV_HANDLER, EXECUTOR, NET_HANDLER to specify which process this function is
		being called from
No return value.
*/
void shm_aux_init (process_t process)
{
	int fd_shm; //file descriptor of the memory-mapped shared memory
	char sname[SNAME_SIZE]; //for holding semaphore names
	
	if (process == NET_HANDLER) {
		
		//mutual exclusion semaphore, gp_sem with initial value = 0
		if ((gp_sem = sem_open(GP_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED) {
			error("sem_open: gamepad_mutex@net_handler");
		}
		
		//mutual exclusion semaphore, rd_sem with initial value = 1
		if ((rd_sem = sem_open(RD_MUTEX_NAME, O_CREAT, 0660, 1)) == SEM_FAILED) {
			error("sem_open: robot_desc_mutex@net_handler");
		}
		
		//create gamepad shm block
		if ((fd_shm = shm_open(GPAD_SHM_NAME, O_RDWR | O_CREAT, 0660)) == -1) {
			error("shm_open gamepad_shm: @net_handler");
		}
		if (ftruncate(fd_shm, sizeof(gamepad_t)) == -1) {
			error("ftruncate gamepad_shm: @net_handler");
		}
		if ((gp_ptr = mmap(NULL, sizeof(gamepad_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
			error("mmap gamepad_shm: @net_handler");
		}
		if (close(fd_shm) == -1) {
			error("close gamepad_shm: @net_handler");
		}
		
		//create robot description shm block
		if ((fd_shm = shm_open(ROBOT_DESC_SHM_NAME, O_RDWR | O_CREAT, 0660)) == -1) {
			error("shm_open robot_desc_shm: @net_handler");
		}
		if (ftruncate(fd_shm, sizeof(robot_desc_t)) == -1) {
			error("ftruncate robot_desc_shm: @net_handler");
		}
		if ((rd_ptr = mmap(NULL, sizeof(robot_desc_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
			error("mmap robot_desc_shm: @net_handler");
		}
		if (close(fd_shm) == -1) {
			error("close robot_desc_shm: @net_handler");
		}
		
		//initialize everything
		gp_ptr->buttons = 0;
		for (int i = 0; i < 4; i++) {
			gp_ptr->joysticks[i] = 0.0;
		}
		rd_ptr->fields[STATE] = NOMINAL;
		rd_ptr->fields[RUN_MODE] = IDLE;
		rd_ptr->fields[DAWN] = DISCONNECTED;
		rd_ptr->fields[SHEPHERD] = DISCONNECTED;
		rd_ptr->fields[GAMEPAD] = DISCONNECTED;
		rd_ptr->fields[TEAM] = BLUE; //arbitrary
		
		//initialization complete; set catalog_mutex to 1 indicating shm segment available for client(s)
		if (sem_post(gp_sem) == -1) {
			error("sem_post: gamepad_mutex@net_handler");
		}
	} else {
		//mutual exclusion semaphore, gamepad_sem
		if ((gp_sem = sem_open(GP_MUTEX_NAME, 0, 0, 0)) == SEM_FAILED) {
			error("sem_open: gamepad_mutex@client");
		}
		
		//mutual exclusion semaphore, rd_sem
		if ((rd_sem = sem_open(RD_MUTEX_NAME, 0, 0, 0)) == SEM_FAILED) {
			error("sem_open: robot_desc_mutex@client");
		}
		
		
		//open gamepad shm block and map to client process virtual memory
		if ((fd_shm = shm_open(GPAD_SHM_NAME, O_RDWR, 0)) == -1) { //no O_CREAT
			error("shm_open: @client");
		}
		if ((gp_ptr = mmap(NULL, sizeof(gamepad_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
			error("mmap: @client");
		}
		if (close(fd_shm) == -1) {
			error("close: @client");
		}
		
		//open robot desc shm block and map to client process virtual memory
		if ((fd_shm = shm_open(ROBOT_DESC_SHM_NAME, O_RDWR, 0)) == -1) { //no O_CREAT
			error("shm_open: @client");
		}
		if ((rd_ptr = mmap(NULL, sizeof(robot_desc_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
			error("mmap: @client");
		}
		if (close(fd_shm) == -1) {
			error("close: @client");
		}
	}
}

/*
Call this function if process no longer wishes to connect to auxiliary shared memory wrapper
No other actions will work after this function is called
Newtork handler is responsible for marking shared memory blocks and semaphores for destruction after all detach
	- process: one of DEV_HANDLER, EXECUTOR, NET_HANDLER to specify which process this function is
		being called from
No return value.
*/
void shm_aux_stop (process_t process)
{
	char sname[SNAME_SIZE]; //holding semaphore names
	
	//unmap the gamepad and robot desc shm blocks
	if (munmap(gp_ptr, sizeof(gamepad_t)) == -1) {
		(process == NET_HANDLER) ? error("munmap: @net_handler") : error("munmap: @client");
	}
	if (munmap(rd_ptr, sizeof(robot_desc_t)) == -1) {
		(process == NET_HANDLER) ? error("munmap: @net_handler") : error("munmap: @client");
	}
	
	//close both semaphores
	if (sem_close(gp_sem) == -1) {
		(process == NET_HANDLER) ? error("sem_close: gamepad_mutex@net_handler") : error("sem_close: gamepad_mutex@client");
	}
	if (sem_close(rd_sem) == -1) {
		(process == NET_HANDLER) ? error("sem_close: robot_desc_mutex@net_handler") : error("sem_close: robot_desc_mutex@client");
	}
	
	//the network handler is also responsible for unlinking everything
	if (process == NET_HANDLER) {
		//unlink shared memory blocks
		if (shm_unlink(GPAD_SHM_NAME) == -1) {
			error("shm_unlink");
		}
		if (shm_unlink(ROBOT_DESC_SHM_NAME) == -1) {
			error("shm_unlink");
		}
		
		//unlink semaphores
		if (sem_unlink(GP_MUTEX_NAME) == -1) {
			error("sem_unlink: gamepad_mutex");
		}
		if (sem_unlink(RD_MUTEX_NAME) == -1) {
			error("sem_unlink: robot_desc_mutex");
		}
	}
}

/*
This function writes the specified value into the specified field.
Blocks on the robot description semaphore.
	- field: one of the robot_desc_val_t's defined above to write val to
	- val: one of the robot_desc_vals defined above to write to the specified field
No return value.
*/
void robot_desc_write (robot_desc_field_t field, robot_desc_val_t val);
{
	//wait on rd_sem
	if (sem_wait(rd_sem) == -1) {
		error("sem_wait: robot_desc_mutex");
	}
	
	//write the val into the field
	rd_ptr->fields[FIELD] = val;
	
	//release rd_sem
	if (sem_post(rd_sem) == -1) {
		error("sem_post: robot_desc_mutex");
	}
}

/*
This function reads the specified field.
Blocks on the robot description semaphore.
	- field: one of the robot_desc_val_t's defined above to read from
Returns one of the robot_desc_val_t's defined above that is the current value of that field.
*/
robot_desc_val_t robot_desc_read (robot_desc_field_t field)
{
	robot_desc_val_t ret;
	
	//wait on rd_sem
	if (sem_wait(rd_sem) == -1) {
		error("sem_wait: robot_desc_mutex");
	}
	
	ret = rd_ptr->fields[FIELD];
	
	//release rd_sem
	if (sem_post(rd_sem) == -1) {
		error("sem_post: robot_desc_mutex");
	}
	
	return ret;
}

/*
This function writes the given state of the gamepad to shared memory.
Blocks on both the gamepad semaphore and device description semaphore (to check if gamepad connected).
	- pressed_buttons: a 32-bit bitmap that corresponds to which buttons are currently pressed
		(only the first NUM_GAMEPAD_BUTTONS bits used, since there are NUM_GAMEPAD_BUTTONS buttons)
	- joystick_vals: array of 4 floats that contain the values to write to the joystick
No return value.
*/
void gamepad_write (uint32_t pressed_buttons, float *joystick_vals)
{
	//wait on rd_sem
	if (sem_wait(rd_sem) == -1) {
		error("sem_wait: robot_desc_mutex");
	}
	
	//if no gamepad connected, then release rd_sem and return
	if (robot_desc_read(GAMEPAD) == DISCONNECTED) {
		log_runtime(WARN, "tried to write, but no gamepad connected");
		if (sem_post(rd_sem) == -1) {
			error("sem_post: robot_desc_mutex");
		}
		return;
	}
	
	//release rd_sem
	if (sem_post(rd_sem) == -1) {
		error("sem_post: robot_desc_mutex");
	}
	
	//wait on gp_sem
	if (sem_wait(gp_sem) == -1) {
		error("sem_wait: gamepad_mutex");
	}
	
	gp_ptr->buttons = pressed_buttons;
	for (int i = 0; i < 4; i++) {
		gp_ptr->joysticks[i] = joystick_vals[i];
	}
	
	//release gp_sem
	if (sem_post(gp_sem) == -1) {
		error("sem_post: gamepad_mutex");
	}
}

/*
This function reads the current state of the gamepad to the provided pointers.
Blocks on both the gamepad semaphore and device description semaphore (to check if gamepad connected).
	- pressed_buttons: pointer to 32-bit bitmap to which the current button bitmap state will be read into
	- joystick_vals: array of 4 floats to which the current joystick states will be read into
No return value.
*/
void gamepad_read (uint32_t &pressed_buttons, float *joystick_vals)
{
	//wait on rd_sem
	if (sem_wait(rd_sem) == -1) {
		error("sem_wait: robot_desc_mutex");
	}
	
	//if no gamepad connected, then release rd_sem and return
	if (robot_desc_read(GAMEPAD) == DISCONNECTED) {
		log_runtime(WARN, "tried to read, but no gamepad connected");
		if (sem_post(rd_sem) == -1) {
			error("sem_post: robot_desc_mutex");
		}
		return;
	}
	
	//release rd_sem
	if (sem_post(rd_sem) == -1) {
		error("sem_post: robot_desc_mutex");
	}
	
	//wait on gp_sem
	if (sem_wait(gp_sem) == -1) {
		error("sem_wait: gamepad_mutex");
	}
	
	*pressed_buttons = gp_ptr->buttons;
	for (int i = 0; i < 4; i++) {
		joystick_vals[i] = gp_ptr->joysticks[i];
	}
	
	//release gp_sem
	if (sem_post(gp_sem) == -1) {
		error("sem_post: gamepad_mutex");
	}
}
