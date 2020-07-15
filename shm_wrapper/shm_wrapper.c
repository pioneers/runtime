#include "shm_wrapper.h"

#define SNAME_SIZE 32 //size of buffers that hold semaphore names, in bytes

//names of various objects used in shm_wrapper; should not be used outside of shm_wrapper and shm_process
#define CATALOG_MUTEX_NAME "/ct-mutex"  //name of semaphore used as a mutex on the catalog
#define PMAP_MUTEX_NAME "/pmap-mutex"   //name of semaphore used as a mutex on the param bitmap
#define DEV_SHM_NAME "/dev-shm"      //name of shared memory block across devices

#define GPAD_SHM_NAME "/gp-shm"         //name of shared memory block for gamepad
#define ROBOT_DESC_SHM_NAME "/rd-shm"   //name of shared memory block for robot description
#define GP_MUTEX_NAME "/gp-sem"         //name of semaphore used as mutex over gamepad shm
#define RD_MUTEX_NAME "/rd-sem"         //name of semaphore used as mutex over robot description shm

// *********************************** WRAPPER-SPECIFIC GLOBAL VARS **************************************** //

dual_sem_t sems[MAX_DEVICES];  //array of semaphores, two for each possible device (one for data and one for commands)
dev_shm_t *dev_shm_ptr;        //points to memory-mapped shared memory block for device data and commands
sem_t *catalog_sem;            //semaphore used as a mutex on the catalog
sem_t *pmap_sem;               //semaphore used as a mutex on the param bitmap

gamepad_shm_t *gp_shm_ptr;     //points to memory-mapped shared memory block for gamepad
robot_desc_shm_t *rd_shm_ptr;  //points to memory-mapped shared memory block for robot description
sem_t *gp_sem;                 //semaphore used as a mutex on the gamepad
sem_t *rd_sem;                 //semaphore used as a mutex on the robot description

// ******************************************* SEMAPHORE UTILITIES **************************************** //

static void my_sem_wait (sem_t *sem, char *sem_desc)
{
	if (sem_wait(sem) == -1) {
		log_printf(ERROR, "sem_wait: %s. %s", sem_desc, strerror(errno));
	}
}

static void my_sem_post (sem_t *sem, char *sem_desc)
{
	if (sem_post(sem) == -1) {
		log_printf(ERROR, "sem_post: %s. %s", sem_desc, strerror(errno));
	}
}

static sem_t *my_sem_open (char *sem_name, char *sem_desc)
{
	sem_t *ret;
	if ((ret = sem_open(sem_name, 0, 0, 0)) == SEM_FAILED) {
		log_printf(FATAL, "sem_open: %s. %s", sem_desc, strerror(errno));
		exit(1);
	}
	return ret;
}

static void my_sem_close (sem_t *sem, char *sem_desc)
{
	if (sem_close(sem) == -1) {
		log_printf(ERROR, "sem_close: %s. %s", sem_desc, strerror(errno));
	}
}

// ******************************************** HELPER FUNCTIONS ****************************************** //

//function for generating device data and command semaphore names
static void generate_sem_name (stream_t stream, int dev_ix, char *name)
{
	if (stream == DATA) {
		sprintf(name, "/data_sem_%d", dev_ix);
	} else if (stream == COMMAND) {
		sprintf(name, "/command_sem_%d", dev_ix);
	}
}

//function that actually reads a value from device shm blocks (does not perform catalog check)
static void device_read_helper (int dev_ix, process_t process, stream_t stream, uint32_t params_to_read, param_val_t *params)
{
	//grab semaphore for the appropriate stream and device
	if (stream == DATA) {
		my_sem_wait(sems[dev_ix].data_sem, "data sem @device_read");
	} else {
		my_sem_wait(sems[dev_ix].command_sem, "command sem @device_read");
	}

	//read all requested params
	for (int i = 0; i < MAX_PARAMS; i++) {
		if (params_to_read & (1 << i)) {
			params[i] = dev_shm_ptr->params[stream][dev_ix][i];
		}
	}

	//if the device handler has processed the command, then turn off the change
	//if stream = downstream and process = dev_handler then also update params bitmap
	if (process == DEV_HANDLER && stream == COMMAND) {
		//wait on pmap_sem
		my_sem_wait(pmap_sem, "pmap_sem @device_read");
	
		dev_shm_ptr->pmap[0] &= (~(1 << dev_ix)); //turn off changed device bit in pmap[0]
		dev_shm_ptr->pmap[dev_ix + 1] &= (~params_to_read); //turn off bits for params that were changed and then read in pmap[dev_ix + 1]
	
		//release pmap_sem
		my_sem_post(pmap_sem, "pmap_sem @device_read");
	}

	//release semaphore for appropriate stream and device
	if (stream == DATA) {
		my_sem_post(sems[dev_ix].data_sem, "data sem @device_read");
	} else {
		my_sem_post(sems[dev_ix].command_sem, "command sem @device_read");
	}
}

//function that actually writes a value to device shm blocks (does not perform catalog check)
static void device_write_helper (int dev_ix, process_t process, stream_t stream, uint32_t params_to_write, param_val_t *params)
{
	//grab semaphore for the appropriate stream and device
	if (stream == DATA) {
		my_sem_wait(sems[dev_ix].data_sem, "data sem @device_write");
	} else {
		my_sem_wait(sems[dev_ix].command_sem, "command sem @device_write");
	}
	
	//write all requested params
	for (int i = 0; i < MAX_PARAMS; i++) {
		if (params_to_write & (1 << i)) {
			dev_shm_ptr->params[stream][dev_ix][i] = params[i];
		}
	}
	
	//turn on flag if executor is sending a command to the device
	//if stream = downstream and process = executor then also update params bitmap
	if (process == EXECUTOR && stream == COMMAND) {
		//wait on pmap_sem
		my_sem_wait(pmap_sem, "pmap_sem @device_write");
		
		dev_shm_ptr->pmap[0] |= (1 << dev_ix); //turn on changed device bit in pmap[0]
		dev_shm_ptr->pmap[dev_ix + 1] |= params_to_write; //turn on bits for params that were written in pmap[dev_ix + 1]
		
		//release pmap_sem
		my_sem_post(pmap_sem, "pmap_sem @device_write");
	}
	
	//release semaphore for appropriate stream and device
	if (stream == DATA) {
		my_sem_post(sems[dev_ix].data_sem, "data sem @device_write");
	} else {
		my_sem_post(sems[dev_ix].command_sem, "command sem @device_write");
	}
}

// ************************************ PUBLIC PRINTING UTILITIES ***************************************** //

//function for printing bitmap that is NUM_BITS long
static void print_bitmap (int num_bits, uint32_t bitmap) 
{
	for (int i = 0; i < num_bits; i++) {
		printf("%d", (bitmap & (1 << i)) ? 1 : 0);
	}
	printf("\n");
}

void print_pmap ()
{
	uint32_t pmap[MAX_DEVICES + 1];
	get_param_bitmap(pmap);
	
	printf("Changed devices: ");
	print_bitmap(MAX_DEVICES, pmap[0]);
	
	printf("Changed params:\n");
	for (int i = 1; i < 33; i++) {
		if (pmap[0] & (1 << (i - 1))) {
			printf("\tDevice %d: ", i - 1);
			print_bitmap(MAX_PARAMS, pmap[i]);
		}
	}
}

void print_dev_ids ()
{
	dev_id_t dev_ids[MAX_DEVICES];
	uint32_t catalog;
	
	get_device_identifiers(dev_ids);
	get_catalog(&catalog);
	
	if (catalog == 0) {
		printf("no connected devices\n");
	} else {
		for (int i = 0; i < MAX_DEVICES; i++) {
			if (catalog & (1 << i)) {
				printf("dev_ix = %d: type = %d, year = %d, uid = %llu\n", i, dev_ids[i].type, dev_ids[i].year, dev_ids[i].uid);
			}
		}
	}
}

void print_catalog ()
{
	uint32_t catalog;
	
	get_catalog(&catalog);
	print_bitmap(MAX_DEVICES, catalog);
}

void print_params (uint32_t devices)
{
	dev_id_t dev_ids[MAX_DEVICES];
	uint32_t catalog;
	device_t *device;
	
	get_device_identifiers(dev_ids);
	get_catalog(&catalog);
	
	for (int i = 0; i < MAX_DEVICES; i++) {
		if ((catalog & (1 << i)) && (devices & (1 << i))) {
			device = get_device(dev_ids[i].type);
			if (device == NULL) {
				printf("Device at index %d with type %d is invalid\n", i, dev_ids[i].type);
				continue;
			}
			printf("dev_ix = %d: name = %s, type = %d, year = %d, uid = %llu\n", i, device->name, dev_ids[i].type, dev_ids[i].year, dev_ids[i].uid);
			
			for (int s = 0; s < 2; s++) {
				//print out the stream header
				if (s == 0) { printf("\tDATA stream:\n"); }
				else if (s == 1) { printf("\tCOMMAND stream:\n"); }
				
				//print all params for the device for that stream
				for (int j = 0; j < device->num_params; j++) {
					float val;
					char *param_type = device->params[j].type;
					if (strcmp(param_type, "int") == 0) {
						val = dev_shm_ptr->params[s][i][j].p_i;
					}
					else if (strcmp(param_type, "float") == 0) {
						val = dev_shm_ptr->params[s][i][j].p_f;
					}
					else if (strcmp(param_type, "bool") == 0) {
						val = dev_shm_ptr->params[s][i][j].p_b;
					}
					else {
						printf("Invalid parameter type %s\n", param_type);
						continue;
					}
					printf("\t\tparam_idx = %d, name = %s, type = %s, value = %f\n", j, device->params[j].name, param_type, val);
				}
			}
		}
	}
}

void print_robot_desc ()
{
	//since there's no get_robot_desc function (we don't need it, and hides the implementation from users)
	//we need to acquire the semaphore for the print
	
	//wait on rd_sem
	my_sem_wait(rd_sem, "robot_desc_mutex (in print)");
	
	printf("Current Robot Description:\n");
	for (int i = 0; i < NUM_DESC_FIELDS; i++) {
		switch (i) {
			case RUN_MODE:
				printf("\tRUN_MODE = %s\n", (rd_shm_ptr->fields[RUN_MODE] == IDLE) ? "IDLE" : 
					((rd_shm_ptr->fields[RUN_MODE] == AUTO) ? "AUTO" : ((rd_shm_ptr->fields[RUN_MODE] == TELEOP) ? "TELEOP" : "CHALLENGE")));
				break;
			case DAWN:
				printf("\tDAWN = %s\n", (rd_shm_ptr->fields[DAWN] == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
				break;
			case SHEPHERD:
				printf("\tSHEPHERD = %s\n", (rd_shm_ptr->fields[SHEPHERD] == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
				break;
			case GAMEPAD:
				printf("\tGAMEPAD = %s\n", (rd_shm_ptr->fields[GAMEPAD] == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
				break;
			case START_POS:
				printf("\tSTART_POS = %s\n", (rd_shm_ptr->fields[START_POS] == LEFT) ? "LEFT" : "RIGHT");
				break;
			default:
				printf("unknown field\n");
		}
	}
	printf("\n");
	fflush(stdout);
	
	//release rd_sem
	my_sem_post(rd_sem, "robot_desc_mutex (in print)");
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
	my_sem_wait(gp_sem, "gamepad_mutex (in print)");
	
	//only print pushed buttons (so we don't print out 22 lines of output each time we all this function)
	printf("Current Gamepad State:\n\tPushed Buttons:\n");
	for (int i = 0; i < NUM_GAMEPAD_BUTTONS; i++) {
		if (gp_shm_ptr->buttons & (1 << i)) {
			printf("\t\t%s\n", button_names[i]);
		}
	}
	printf("\tJoystick Positions:\n");
	//print joystick positions
	for (int i = 0; i < 4; i++) {
		printf("\t\t %s = %f\n", joystick_names[i], gp_shm_ptr->joysticks[i]);
	}
	printf("\n");
	fflush(stdout);
	
	//release rd_sem
	my_sem_post(gp_sem, "gamepad_mutex (in print)");
}


// ************************************ PUBLIC WRAPPER FUNCTIONS ****************************************** //

/*
Call this function from every process that wants to use the shared memory wrapper
Should be called before any other action happens
The device handler process is responsible for initializing the catalog and updated param bitmap
	- process: one of DEV_HANDLER, EXECUTOR, NET_HANDLER to specify which process this function is being
		called from
No return value.
*/
void shm_init ()
{
	int fd_shm; //file descriptor of the memory-mapped shared memory
	char sname[SNAME_SIZE]; //for holding semaphore names
	
	//open all the semaphores
	catalog_sem = my_sem_open(CATALOG_MUTEX_NAME, "catalog mutex");
	pmap_sem = my_sem_open(PMAP_MUTEX_NAME, "pmap mutex");
	gp_sem = my_sem_open(GP_MUTEX_NAME, "gamepad mutex");
	rd_sem = my_sem_open(RD_MUTEX_NAME, "robot desc mutex");
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
	
	//open shared memory block and map to client process virtual memory
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
}

/*
Call this function if process no longer wishes to connect to shared memory wrapper
No other actions will work after this function is called
Device handler is responsible for marking shared memory block and semaphores for destruction after all detach
	- process: one of DEV_HANDLER, EXECUTOR, NET_HANDLER to specify which process this function is being
		called from
No return value.
*/
void shm_stop ()
{
	//close all the semaphores
	for (int i = 0; i < MAX_DEVICES; i++) {
		my_sem_close(sems[i].data_sem, "data sem");
		my_sem_close(sems[i].command_sem, "command sem");
	}
	my_sem_close(catalog_sem, "catalog sem");
	my_sem_close(pmap_sem, "pmap sem");
	my_sem_close(gp_sem, "gamepad_mutex");
	my_sem_close(rd_sem, "robot_desc_mutex");
	
	//unmap all shared memory blocks
	if (munmap(dev_shm_ptr, sizeof(dev_shm_t)) == -1) {
		log_printf(ERROR, "munmap: dev_shm. %s", strerror(errno));
	}
	if (munmap(gp_shm_ptr, sizeof(gamepad_shm_t)) == -1) {
		log_printf(ERROR, "munmap: gp_shm. %s", strerror(errno));
	}
	if (munmap(rd_shm_ptr, sizeof(robot_desc_shm_t)) == -1) {
		log_printf(ERROR, "munmap: robot_desc_shm. %s", strerror(errno));
	}
}

/*
Should only be called from device handler
Selects the next available device index and assigns the newly connected device to that location. 
Turns on the associated bit in the catalog.
	- dev_type: the type of the device being connected e.g. LIMITSWITCH, LINEFOLLOWER, etc; see device_config.h
	- dev_year: year of device manufacture
	- dev_uid: the unique, random 64-bit uid assigned to the device when flashing
	- dev_ix: the index that the device was assigned will be put here
No return value.
*/
void device_connect (uint16_t dev_type, uint8_t dev_year, uint64_t dev_uid, int *dev_ix)
{	
	//wait on catalog_sem
	my_sem_wait(catalog_sem, "catalog_sem");
	
	//find a valid dev_ix
	for (*dev_ix = 0; *dev_ix < MAX_DEVICES; (*dev_ix)++) {
		if (!(dev_shm_ptr->catalog & (1 << *dev_ix))) { //if the spot at dev_ix is free
			break;
		}
	}
	if (*dev_ix == MAX_DEVICES) {
		log_printf(ERROR, "too many devices, connection unsuccessful");
		my_sem_post(catalog_sem, "catalog_sem"); //release the catalog semaphore
		return;
	}
	
	//wait on associated data and command sems
	my_sem_wait(sems[*dev_ix].data_sem, "data_sem");
	my_sem_wait(sems[*dev_ix].command_sem, "command_sem");
	
	//fill in dev_id for that device with provided values
	dev_shm_ptr->dev_ids[*dev_ix].type = dev_type;
	dev_shm_ptr->dev_ids[*dev_ix].year = dev_year;
	dev_shm_ptr->dev_ids[*dev_ix].uid = dev_uid;
	
	//update the catalog
	dev_shm_ptr->catalog |= (1 << *dev_ix);

	//reset param values to 0
	for (int i = 0; i < MAX_PARAMS; i++) {
		dev_shm_ptr->params[DATA][*dev_ix][i] = (const param_val_t) {0};
		dev_shm_ptr->params[COMMAND][*dev_ix][i] = (const param_val_t) {0};
	}
	
	//release associated data and command sems
	my_sem_post(sems[*dev_ix].data_sem, "data_sem");
	my_sem_post(sems[*dev_ix].command_sem, "command_sem");
	
	//release catalog_sem
	my_sem_post(catalog_sem, "catalog_sem");
}

/*
Should only be called from device handler
Disconnects a device with a given index by turning off the associated bit in the catalog.
	- dev_ix: index of device in catalog to be disconnected
No return value.
*/
void device_disconnect (int dev_ix)
{	
	//wait on catalog_sem
	my_sem_wait(catalog_sem, "catalog_sem");
	
	//wait on associated data and command sems
	my_sem_wait(sems[dev_ix].data_sem, "data_sem");
	my_sem_wait(sems[dev_ix].command_sem, "command_sem");
	
	//wait on pmap_sem
	my_sem_wait(pmap_sem, "pmap_sem");

	//update the catalog
	dev_shm_ptr->catalog &= (~(1 << dev_ix));

	//reset param map values to 0
	dev_shm_ptr->pmap[0] &= (~(1 << dev_ix));   //reset the changed bit flag in pmap[0]
	dev_shm_ptr->pmap[dev_ix + 1] = 0;          //turn off all changed bits for the device
	
	//release pmap_sem
	my_sem_post(pmap_sem, "pmap_sem");
	
	//release associated upstream and downstream sems
	my_sem_post(sems[dev_ix].data_sem, "data_sem");
	my_sem_post(sems[dev_ix].command_sem, "command_sem");
	
	//release catalog_sem
	my_sem_post(catalog_sem, "catalog_sem");
}

/*	
Should be called from every process wanting to read the device data
Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
Grabs either one or two semaphores depending on calling process and stream requested.
	- dev_ix: device index of the device whose data is being requested
	- process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
	- stream: the requested block to read from, one of DATA, COMMAND
	- params_to_read: bitmap representing which params to be read 
		(nonexistent params should have corresponding bits set to 0)
	- params: pointer to array of param_val_t's that is at least as long as highest requested param number
		device data will be read into the corresponding param_val_t's
No return value.
*/
void device_read (int dev_ix, process_t process, stream_t stream, uint32_t params_to_read, param_val_t *params)
{
	
	//check catalog to see if dev_ix is valid, if not then return immediately
	if (!(dev_shm_ptr->catalog & (1 << dev_ix))) {
		log_printf(ERROR, "no device at dev_ix = %d, read failed", dev_ix);
		return;
	}
	
	//call the helper to do the actual reading
	device_read_helper(dev_ix, process, stream, params_to_read, params);
}

/*
This function is the exact same as the above function, but instead uses the 64-bit device UID to identify
the device that should be read, rather than the device index.
*/
void device_read_uid (uint64_t dev_uid, process_t process, stream_t stream, uint32_t params_to_read, param_val_t *params)
{
	int dev_ix = -1;
	
	//check catalog and dev_shm_ptr->dev_ids to determine if device exists
	for (int i = 0; i < MAX_DEVICES; i++) {
		if ((dev_shm_ptr->catalog & (1 << i)) && (dev_shm_ptr->dev_ids[i].uid == dev_uid)) {
			dev_ix = i;
			break;
		}
	}
	
	//if device doesn't exist, return immediately
	if (dev_ix == -1) {
		log_printf(ERROR, "no device at dev_uid = %llu, read failed", dev_uid);
		return;
	}
	
	//call the helper to do the actual reading
	device_read_helper(dev_ix, process, stream, params_to_read, params);
}

/*	
Should be called from every process wanting to write to the device data
Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
Grabs either one or two semaphores depending on calling process and stream requested.
	- dev_ix: device index of the device whose data is being written
	- process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
	- stream: the requested block to write to, one of DATA, COMMAND
	- params_to_write: bitmap representing which params to be written
		(nonexistent params should have corresponding bits set to 0)
	- params: pointer to array of param_val_t's that is at least as long as highest requested param number
		device data will be written into the corresponding param_val_t's
No return value.
*/
void device_write (int dev_ix, process_t process, stream_t stream, uint32_t params_to_write, param_val_t *params)
{
	
	//check catalog to see if dev_ix is valid, if not then return immediately
	if (!(dev_shm_ptr->catalog & (1 << dev_ix))) {
		log_printf(ERROR, "no device at dev_ix = %d, write failed", dev_ix);
		return;
	}
	
	//call the helper to do the actual reading
	device_write_helper(dev_ix, process, stream, params_to_write, params);
}

/*
This function is the exact same as the above function, but instead uses the 64-bit device UID to identify
the device that should be written, rather than the device index.
*/
void device_write_uid (uint64_t dev_uid, process_t process, stream_t stream, uint32_t params_to_write, param_val_t *params)
{
	int dev_ix = -1;
	
	//check catalog and dev_shm_ptr->dev_ids to determine if device exists
	for (int i = 0; i < MAX_DEVICES; i++) {
		if ((dev_shm_ptr->catalog & (1 << i)) && (dev_shm_ptr->dev_ids[i].uid == dev_uid)) {
			dev_ix = i;
			break;
		}
	}
	
	//if device doesn't exist, return immediately
	if (dev_ix == -1) {
		log_printf(ERROR, "no device at dev_uid = %llu, write failed", dev_uid);
		return;
	}
	
	//call the helper to do the actual reading
	device_write_helper(dev_ix, process, stream, params_to_write, params);
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
	my_sem_wait(rd_sem, "robot_desc_mutex");
	
	//read the value out, and turn off the appropriate element
	ret = rd_shm_ptr->fields[field];
	
	//release rd_sem
	my_sem_post(rd_sem, "robot_desc_mutex");
	
	return ret;
}

/*
This function writes the specified value into the specified field.
Blocks on the robot description semaphore.
	- field: one of the robot_desc_val_t's defined above to write val to
	- val: one of the robot_desc_vals defined above to write to the specified field
No return value.
*/
void robot_desc_write (robot_desc_field_t field, robot_desc_val_t val)
{	
	//wait on rd_sem
	my_sem_wait(rd_sem, "robot_desc_mutex");
	
	//write the val into the field, and set appropriate pending element to 1
	rd_shm_ptr->fields[field] = val;
	
	//release rd_sem
	my_sem_post(rd_sem, "robot_desc_mutex");
}

/*
This function reads the current state of the gamepad to the provided pointers.
Blocks on both the gamepad semaphore and device description semaphore (to check if gamepad connected).
	- pressed_buttons: pointer to 32-bit bitmap to which the current button bitmap state will be read into
	- joystick_vals: array of 4 floats to which the current joystick states will be read into
No return value.
*/
void gamepad_read (uint32_t *pressed_buttons, float *joystick_vals)
{
	//wait on rd_sem
	my_sem_wait(rd_sem, "robot_desc_mutex");
	
	//if no gamepad connected, then release rd_sem and return
	if (rd_shm_ptr->fields[GAMEPAD] == DISCONNECTED) {
		log_printf(ERROR, "tried to read, but no gamepad connected");
		my_sem_post(rd_sem, "robot_desc_mutex");
		return;
	}
	
	//release rd_sem
	my_sem_post(rd_sem, "robot_desc_mutex");
	
	//wait on gp_sem
	my_sem_wait(gp_sem, "gamepad_mutex");
	
	*pressed_buttons = gp_shm_ptr->buttons;
	for (int i = 0; i < 4; i++) {
		joystick_vals[i] = gp_shm_ptr->joysticks[i];
	}
	
	//release gp_sem
	my_sem_post(gp_sem, "gamepad_mutex");
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
	my_sem_wait(rd_sem, "robot_desc_mutex");
	
	//if no gamepad connected, then release rd_sem and return
	if (rd_shm_ptr->fields[GAMEPAD] == DISCONNECTED) {
		log_printf(ERROR, "tried to write, but no gamepad connected");
		my_sem_post(rd_sem, "robot_desc_mutex");
		return;
	}
	
	//release rd_sem
	my_sem_post(rd_sem, "robot_desc_mutex");
	
	//wait on gp_sem
	my_sem_wait(gp_sem, "gamepad_mutex");
	
	gp_shm_ptr->buttons = pressed_buttons;
	for (int i = 0; i < 4; i++) {
		gp_shm_ptr->joysticks[i] = joystick_vals[i];
	}
	
	//release gp_sem
	my_sem_post(gp_sem, "gamepad_mutex");
}

/*
Should be called from all processes that want to know current state of the param bitmap (i.e. device handler)
Blocks on the param bitmap semaphore for obvious reasons
	- bitmap: pointer to array of 17 32-bit integers to copy the bitmap into
No return value.
*/
void get_param_bitmap (uint32_t *bitmap)
{
	//wait on pmap_sem
	my_sem_wait(pmap_sem, "pmap_sem");
	
	for (int i = 0; i < MAX_DEVICES + 1; i++) {
		bitmap[i] = dev_shm_ptr->pmap[i];
	}
	
	//release pmap_sem
	my_sem_post(pmap_sem, "pmap_sem");
}

/*
Should be called from all processes that want to know device identifiers of all currently connected devices
Blocks on catalog semaphore for obvious reasons
	- dev_ids: pointer to array of dev_id_t's to copy the information into
No return value.
*/
void get_device_identifiers (dev_id_t *dev_ids)
{
	//wait on catalog_sem
	my_sem_wait(catalog_sem, "catalog_sem");
	
	for (int i = 0; i < MAX_DEVICES; i++) {
		dev_ids[i] = dev_shm_ptr->dev_ids[i];
	}
	
	//release catalog_sem
	my_sem_post(catalog_sem, "catalog_sem");
}

/*
Should be called from all processes that want to know which dev_ix's are valid
Blocks on catalog semaphore for obvious reasons
	- catalog: pointer to 32-bit integer into which the current catalog will be read into
No return value.
*/
void get_catalog (uint32_t *catalog)
{
	//wait on catalog_sem
	my_sem_wait(catalog_sem, "catalog_sem");
	
	*catalog = dev_shm_ptr->catalog;
	
	//release catalog_sem
	my_sem_post(catalog_sem, "catalog_sem");
}
