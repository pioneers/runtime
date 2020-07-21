#include "shm_wrapper.h"
#include <stdlib.h>

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

//function for printing bitmap that is NUM_BITS long
static void print_bitmap (int num_bits, uint32_t bitmap)
{
	for (int i = 0; i < num_bits; i++) {
		printf("%d", (bitmap & (1 << i)) ? 1 : 0);
	}
	printf("\n");
}

//function that returns dev_ix of specified device if it exists (-1 if it doesn't)
static int get_dev_ix_from_uid (uint64_t dev_uid)
{
	int dev_ix = -1;

	for (int i = 0; i < MAX_DEVICES; i++) {
		if ((dev_shm_ptr->catalog & (1 << i)) && (dev_shm_ptr->dev_ids[i].uid == dev_uid)) {
			dev_ix = i;
			break;
		}
	}
	return dev_ix;
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
		//wait on cmd_map_sem
		my_sem_wait(cmd_map_sem, "cmd_map_sem @device_read");

		dev_shm_ptr->cmd_map[0] &= (~(1 << dev_ix)); //turn off changed device bit in cmd_map[0]
		dev_shm_ptr->cmd_map[dev_ix + 1] &= (~params_to_read); //turn off bits for params that were changed and then read in cmd_map[dev_ix + 1]

		//release cmd_map_sem
		my_sem_post(cmd_map_sem, "cmd_map_sem @device_read");
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
		//wait on cmd_map_sem
		my_sem_wait(cmd_map_sem, "cmd_map_sem @device_write");

		dev_shm_ptr->cmd_map[0] |= (1 << dev_ix); //turn on changed device bit in cmd_map[0]
		dev_shm_ptr->cmd_map[dev_ix + 1] |= params_to_write; //turn on bits for params that were written in cmd_map[dev_ix + 1]

		//release cmd_map_sem
		my_sem_post(cmd_map_sem, "cmd_map_sem @device_write");
	}

	//release semaphore for appropriate stream and device
	if (stream == DATA) {
		my_sem_post(sems[dev_ix].data_sem, "data sem @device_write");
	} else {
		my_sem_post(sems[dev_ix].command_sem, "command sem @device_write");
	}
}

// ************************************ PUBLIC PRINTING UTILITIES ***************************************** //

/*
 * Prints the current values in the command bitmap
 */
void print_cmd_map ()
{
	uint32_t cmd_map[MAX_DEVICES + 1];
	get_cmd_map(cmd_map);

	printf("Changed devices: ");
	print_bitmap(MAX_DEVICES, cmd_map[0]);

	printf("Changed params:\n");
	for (int i = 1; i < 33; i++) {
		if (cmd_map[0] & (1 << (i - 1))) {
			printf("\tDevice %d: ", i - 1);
			print_bitmap(MAX_PARAMS, cmd_map[i]);
		}
	}
}

/*
 * Prints the current values in the subscription bitmap
 */
void print_sub_map ()
{
	uint32_t sub_map[MAX_DEVICES + 1];
	get_sub_requests(sub_map);

	printf("Requested devices: ");
	print_bitmap(MAX_DEVICES, sub_map[0]);

	printf("Requested params:\n");
	for (int i = 1; i < 33; i++) {
		if (sub_map[0] & (1 << (i - 1))) {
			printf("\tDevice %d: ", i - 1);
			print_bitmap(MAX_PARAMS, sub_map[i]);
		}
	}
}

/*
 * Prints the device identification info of the currently attached devices
 */
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

/*
 * Prints the catalog (i.e. how many and at which indices devices are currently attached)
 */
void print_catalog ()
{
	uint32_t catalog;

	get_catalog(&catalog);
	print_bitmap(MAX_DEVICES, catalog);
}

/*
 * Prints the params of the specified devices
 */
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
					switch (device->params[j].type) {
						case INT:
							printf("\t\tparam_idx = %d, name = %s, value = %d\n", j, device->params[j].name, dev_shm_ptr->params[s][i][j].p_i);
							break;
						case FLOAT:
							printf("\t\tparam_idx = %d, name = %s, value = %s\n", j, device->params[j].name, (dev_shm_ptr->params[s][i][j].p_b) ? "True" : "False");
							break;
						case BOOL:
							printf("\t\tparam_idx = %d, name = %s, value = %f\n", j, device->params[j].name, dev_shm_ptr->params[s][i][j].p_f);
							break;
					}
				}
			}
		}
	}
}

/*
 * Prints the current values of the robot description in a human-readable way
 */
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

/*
 * Prints the current state of the gamepad in a human-readable way
 */
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
 * Call this function from every process that wants to use the shared memory wrapper
 * No return value (will exit on fatal errors).
 */
void shm_init ()
{
	int fd_shm; //file descriptor of the memory-mapped shared memory
	char sname[SNAME_SIZE]; //for holding semaphore names

	//open all the semaphores
	catalog_sem = my_sem_open(CATALOG_MUTEX_NAME, "catalog mutex");
	cmd_map_sem = my_sem_open(CMDMAP_MUTEX_NAME, "cmd map mutex");
	sub_map_sem = my_sem_open(SUBMAP_MUTEX_NAME, "sub map mutex");
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

	atexit(shm_stop);
}

/*
 * Call this function if process no longer wishes to connect to shared memory wrapper
 * No other actions will work after this function is called.
 * No return value.
 */
void shm_stop ()
{
	//close all the semaphores
	for (int i = 0; i < MAX_DEVICES; i++) {
		my_sem_close(sems[i].data_sem, "data sem");
		my_sem_close(sems[i].command_sem, "command sem");
	}
	my_sem_close(catalog_sem, "catalog sem");
	my_sem_close(cmd_map_sem, "cmd map sem");
	my_sem_close(sub_map_sem, "sub map sem");
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
 * Should only be called from device handler
 * Selects the next available device index and assigns the newly connected device to that location.
 * Turns on the associated bit in the catalog.
 * Arguments:
 *    - uint8_t dev_type: the type of the device being connected e.g. LIMITSWITCH, LINEFOLLOWER, etc.
 *    - uint8_t dev_year: year of device manufacture
 *    - uint64_t dev_uid: the unique, random 64-bit uid assigned to the device when flashing
 *    - int *dev_ix: the index that the device was assigned will be put here
 * Returns device index of connected device in dev_ix on success; sets *dev_ix = -1 on failure
 */
void device_connect (uint8_t dev_type, uint8_t dev_year, uint64_t dev_uid, int *dev_ix)
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
		*dev_ix = -1;
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
 * Should only be called from device handler
 * Disconnects a device with a given index by turning off the associated bit in the catalog.
 * Arguments
 *    - int dev_ix: index of device in catalog to be disconnected
 * No return value.
 */
void device_disconnect (int dev_ix)
{
	//wait on catalog_sem
	my_sem_wait(catalog_sem, "catalog_sem");

	//wait on associated data and command sems
	my_sem_wait(sems[dev_ix].data_sem, "data_sem");
	my_sem_wait(sems[dev_ix].command_sem, "command_sem");

	//wait on cmd_map_sem
	my_sem_wait(cmd_map_sem, "cmd_map_sem");

	//update the catalog
	dev_shm_ptr->catalog &= (~(1 << dev_ix));

	//reset param map values to 0
	dev_shm_ptr->cmd_map[0] &= (~(1 << dev_ix));   //reset the changed bit flag in cmd_map[0]
	dev_shm_ptr->cmd_map[dev_ix + 1] = 0;          //turn off all changed bits for the device

	//release cmd_map_sem
	my_sem_post(cmd_map_sem, "cmd_map_sem");

	//release associated upstream and downstream sems
	my_sem_post(sems[dev_ix].data_sem, "data_sem");
	my_sem_post(sems[dev_ix].command_sem, "command_sem");

	//release catalog_sem
	my_sem_post(catalog_sem, "catalog_sem");
}

/*
 * Should be called from every process wanting to read the device data
 * Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
 * Arguments:
 *    - int dev_ix: device index of the device whose data is being requested
 *    - process_t process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
 *    - stream_t stream: the requested block to read from, one of DATA, COMMAND
 *    - uint32_t params_to_read: bitmap representing which params to be read  (nonexistent params should have corresponding bits set to 0)
 *    - param_val_t params: pointer to array of param_val_t's that is at least as long as highest requested param number
 *            device data will be read into the corresponding param_val_t's
 * Returns 0 on success, -1 on failure (specified device is not connected in shm)
 */
int device_read (int dev_ix, process_t process, stream_t stream, uint32_t params_to_read, param_val_t *params)
{
	//check catalog to see if dev_ix is valid, if not then return immediately
	if (!(dev_shm_ptr->catalog & (1 << dev_ix))) {
		log_printf(ERROR, "no device at dev_ix = %d, read failed", dev_ix);
		return -1;
	}

	//call the helper to do the actual reading
	device_read_helper(dev_ix, process, stream, params_to_read, params);
	return 0;
}

/*
 * This function is the exact same as the above function, but instead uses the 64-bit device UID to identify
 * the device that should be read, rather than the device index.
 */
int device_read_uid (uint64_t dev_uid, process_t process, stream_t stream, uint32_t params_to_read, param_val_t *params)
{
	int dev_ix;

	//if device doesn't exist, return immediately
	if ((dev_ix = get_dev_ix_from_uid(dev_uid)) == -1) {
		log_printf(ERROR, "no device at dev_uid = %llu, read failed", dev_uid);
		return -1;
	}

	//call the helper to do the actual reading
	device_read_helper(dev_ix, process, stream, params_to_read, params);
	return 0;
}

/*
 * Should be called from every process wanting to write to the device data
 * Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
 * Grabs either one or two semaphores depending on calling process and stream requested.
 * Arguments
 *    - int dev_ix: device index of the device whose data is being written
 *    - process_t process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
 *    - stream_t stream: the requested block to write to, one of DATA, COMMAND
 *    - uint32_t params_to_read: bitmap representing which params to be written (nonexistent params should have corresponding bits set to 0)
 *    - param_val_t params: pointer to array of param_val_t's that is at least as long as highest requested param number
 *            device data will be written into the corresponding param_val_t's
 * Returns 0 on success, -1 on failure (specified device is not connected in shm)
 */
int device_write (int dev_ix, process_t process, stream_t stream, uint32_t params_to_write, param_val_t *params)
{

	//check catalog to see if dev_ix is valid, if not then return immediately
	if (!(dev_shm_ptr->catalog & (1 << dev_ix))) {
		log_printf(ERROR, "no device at dev_ix = %d, write failed", dev_ix);
		return -1;
	}

	//call the helper to do the actual reading
	device_write_helper(dev_ix, process, stream, params_to_write, params);
	return 0;
}

/*
 * This function is the exact same as the above function, but instead uses the 64-bit device UID to identify
 * the device that should be written, rather than the device index.
 */
int device_write_uid (uint64_t dev_uid, process_t process, stream_t stream, uint32_t params_to_write, param_val_t *params)
{
	int dev_ix;

	//if device doesn't exist, return immediately
	if ((dev_ix = get_dev_ix_from_uid(dev_uid)) == -1) {
		log_printf(ERROR, "no device at dev_uid = %llu, write failed", dev_uid);
		return -1;
	}

	//call the helper to do the actual reading
	device_write_helper(dev_ix, process, stream, params_to_write, params);
	return 0;
}

/*
 * Send a sub request to dev_handler for a particular device. Takes care of updating the changed bits.
 * Should only be called by executor and net_handler
 * Arguments:
 *    - uint64_t dev_uid: unique 64-bit identifier of the device
 *    - process_t process: the calling process (will error if not EXECUTOR or NET_HANDLER)
 *    - uint32_t params_to_sub: bitmap representing params to subscribe to (nonexistent params should have corresponding bits set to 0)
 * Returns 0 on success, -1 on failure (unrecognized process, or device is not connect in shm)
 */
int place_sub_request (uint64_t dev_uid, process_t process, uint32_t params_to_sub)
{
	int dev_ix;               //dev_ix that we're operating on
	uint32_t *curr_sub_map;   //sub map that we're operating on

	//validate request and obtain dev_ix, sub_map
	if (process != NET_HANDLER && process != EXECUTOR) {
		log_printf(ERROR, "calling place_sub_request from process %u", process);
		return -1;
	}
	if ((dev_ix = get_dev_ix_from_uid(dev_uid)) == -1) {
		log_printf(ERROR, "no device at dev_uid = %llu, sub request failed", dev_uid);
		return -1;
	}
	curr_sub_map = (process == NET_HANDLER) ? dev_shm_ptr->net_sub_map : dev_shm_ptr->exec_sub_map;

	//wait on sub_map_sem
	my_sem_wait(sub_map_sem, "sub_map_sem");

	//only fill in params_to_sub if it's different from what's already there
	if (curr_sub_map[dev_ix + 1] != params_to_sub) {
		curr_sub_map[dev_ix + 1] = params_to_sub;
		curr_sub_map[0] |= (1 << dev_ix); //turn on corresponding bit
	}

	//release sub_map_sem
	my_sem_post(sub_map_sem, "sub_map_sem");

	return 0;
}

/*
 * Get current subscription requests for all devices. Should only be called by dev_handler
 * Arguments:
 *    - uint32_t sub_map[MAX_DEVICES + 1]: bitwise OR of the executor and net_handler sub_maps that will be put into this provided buffer
 * No return value.
 */
void get_sub_requests (uint32_t sub_map[MAX_DEVICES + 1])
{
	//wait on sub_map_sem
	my_sem_wait(sub_map_sem, "sub_map_sem");

	//bitwise or the changes into sub_map[0]; if no changes then return
	sub_map[0] = dev_shm_ptr->net_sub_map[0] | dev_shm_ptr->exec_sub_map[0];
	if (sub_map[0] == 0) {
		my_sem_post(sub_map_sem, "sub_map_sem");
		return;
	}

	//bitwise OR each valid element together into sub_map[i]
	for (int i = 1; i < MAX_DEVICES + 1; i++) {
		if (dev_shm_ptr->catalog & (1 << (i - 1))) { //if device exists
			sub_map[i] = dev_shm_ptr->net_sub_map[i] | dev_shm_ptr->exec_sub_map[i];
		} else {
			sub_map[i] = 0;
		}
	}

	//reset the change indicator eleemnts in net_sub_map and exec_sub_map
	dev_shm_ptr->net_sub_map[0] = dev_shm_ptr->exec_sub_map[0] = 0;

	//release sub_map_sem
	my_sem_post(sub_map_sem, "sub_map_sem");
}

/*
 * Should be called from all processes that want to know current state of the command bitmap (i.e. device handler)
 * Blocks on the param bitmap semaphore for obvious reasons
 * Arguments:
 *    - uint32_t bitmap[MAX_DEVICES + 1]: pointer to array of 33 32-bit integers to copy the bitmap into
 * No return value.
 */
void get_cmd_map (uint32_t bitmap[MAX_DEVICES + 1])
{
	//wait on cmd_map_sem
	my_sem_wait(cmd_map_sem, "cmd_map_sem");

	for (int i = 0; i < MAX_DEVICES + 1; i++) {
		bitmap[i] = dev_shm_ptr->cmd_map[i];
	}

	//release cmd_map_sem
	my_sem_post(cmd_map_sem, "cmd_map_sem");
}

/*
 * Should be called from all processes that want to know device identifiers of all currently connected devices
 * Blocks on catalog semaphore for obvious reasons
 * Arguments:
 *    - dev_id_t dev_ids[MAX_DEVICES]: pointer to array of dev_id_t's to copy the information into
 * No return value.
 */
void get_device_identifiers (dev_id_t dev_ids[MAX_DEVICES])
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
 * Should be called from all processes that want to know which dev_ix's are valid
 * Blocks on catalog semaphore for obvious reasons
 * Arguments:
 *    - catalog: pointer to 32-bit integer into which the current catalog will be read into
 * No return value.
 */
void get_catalog (uint32_t *catalog)
{
	//wait on catalog_sem
	my_sem_wait(catalog_sem, "catalog_sem");

	*catalog = dev_shm_ptr->catalog;

	//release catalog_sem
	my_sem_post(catalog_sem, "catalog_sem");
}


/*
 * Reads the specified robot description field. Blocks on the robot description semaphore.
 * Arguments:
 *    - field: one of the robot_desc_val_t's defined above to read from
 * Returns one of the robot_desc_val_t's defined in runtime_util that is the current value of the requested field.
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
 * Writes the specified value into the specified field. Blocks on the robot description semaphore.
 * Arguments:
 *    - robot_desc_field_t field: one of the robot_desc_val_t's defined above to write val to
 *    - robot_desc_val_t val: one of the robot_desc_vals defined in runtime_util.c to write to the specified field
 * No return value.
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
 * Reads current state of the gamepad to the provided pointers.
 * Blocks on both the gamepad semaphore and device description semaphore (to check if gamepad connected).
 * Arguments:
 *    - uint32_t pressed_buttons: pointer to 32-bit bitmap to which the current button bitmap state will be read into
 *    - float joystick_vals[4]: array of 4 floats to which the current joystick states will be read into
 * Returns 0 on success, -1 on failure (if gamepad is not connected)
 */
int gamepad_read (uint32_t *pressed_buttons, float joystick_vals[4])
{
	//wait on rd_sem
	my_sem_wait(rd_sem, "robot_desc_mutex");

	//if no gamepad connected, then release rd_sem and return
	if (rd_shm_ptr->fields[GAMEPAD] == DISCONNECTED) {
		log_printf(ERROR, "tried to read, but no gamepad connected");
		my_sem_post(rd_sem, "robot_desc_mutex");
		return -1;
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

	return 0;
}

/*
 * This function writes the given state of the gamepad to shared memory.
 * Blocks on both the gamepad semaphore and device description semaphore (to check if gamepad connected).
 * Arguments:
 *    - uint32_t pressed_buttons: a 32-bit bitmap that corresponds to which buttons are currently pressed
            (only the first 17 bits used, since there are 17 buttons)
 *    - float joystick_vals[4]: array of 4 floats that contain the values to write to the joystick
 * Returns 0 on success, -1 on failuire (if gamepad is not connected)
 */
int gamepad_write (uint32_t pressed_buttons, float joystick_vals[4])
{
	//wait on rd_sem
	my_sem_wait(rd_sem, "robot_desc_mutex");

	//if no gamepad connected, then release rd_sem and return
	if (rd_shm_ptr->fields[GAMEPAD] == DISCONNECTED) {
		log_printf(ERROR, "tried to write, but no gamepad connected");
		my_sem_post(rd_sem, "robot_desc_mutex");
		return -1;
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

	return 0;
}
