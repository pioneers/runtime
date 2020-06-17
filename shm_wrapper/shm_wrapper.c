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

#define CATALOG_MUTEX_NAME "/ct-mutex"  //name of semaphore used as a mutex on the catalog
#define PMAP_MUTEX_NAME "/pmap-mutex"   //name of semaphore used as a mutex on the param bitmap
#define SHARED_MEM_NAME "/dev-shm"      //name of shared memory block across devices

#define SNAME_SIZE 32 //size of buffers that hold semaphore names, in bytes

// ***************************************** PRIVATE TYPEDEFS ********************************************* //

//shared memory has these parts in it
typedef struct shm {
	uint32_t catalog;                                   //catalog of valid devices
	uint32_t pmap[MAX_DEVICES + 1];                     //param bitmap is 17 32-bit integers (changed devices and changed params of devices)
	param_val_t params[2][MAX_DEVICES][MAX_PARAMS];     //all the device parameter info, data and commands
	dev_id_t dev_ids[MAX_DEVICES];                      //all the device identification info
} shm_t;

//two mutex semaphores for each device
typedef struct sems {
	sem_t *data_sem;
	sem_t *command_sem;
} dual_sem_t;

// *********************************** WRAPPER-SPECIFIC GLOBAL VARS **************************************** //

dual_sem_t sems[MAX_DEVICES];       //array of semaphores, two for each possible device (one for data and one for commands)
shm_t *shm_ptr;                     //points to memory-mapped shared memory block
sem_t *catalog_sem;                 //semaphore used as a mutex on the catalog
sem_t *pmap_sem;                    //semaphore used as a mutex on the param bitmap

// ******************************************** HELPER FUNCTIONS ****************************************** //

static void error (char *msg)
{
	perror(msg);
	log_runtime(ERROR, msg); //send the message to the logger
	exit(1);
}

static void generate_sem_name (stream_t stream, int dev_ix, char *name)
{
	if (stream == DATA) {
		sprintf(name, "/data_sem_%d", dev_ix);
	} 
	else if (stream == COMMAND) {
		sprintf(name, "/command_sem_%d", dev_ix);
	}
}

static void print_bitmap (int num_bits, uint32_t bitmap) 
{
	for (int i = 0; i < num_bits; i++) {
		printf("%d", (bitmap & (1 << i)) ? 1 : 0);
	}
	printf("\n");
}

//a few very useful semaphore operation wrapper utilities
static void my_sem_wait (sem_t *sem, char *sem_desc)
{
	if (sem_wait(sem) == -1) {
		char msg[64];
		sprintf(msg, "sem_wait: %s", sem_desc);
		error(msg);
	}
}

static void my_sem_post (sem_t *sem, char *sem_desc)
{
	if (sem_post(sem) == -1) {
		char msg[64];
		sprintf(msg, "sem_post: %s", sem_desc);
		error(msg);
	}
}

static void my_sem_close (sem_t *sem, char *sem_desc)
{
	if (sem_close(sem) == -1) {
		char msg[64];
		sprintf(msg, "sem_close: %s", sem_desc);
		error(msg);
	}
}

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
			params[i] = shm_ptr->params[stream][dev_ix][i];
		}
	}

	//if the device handler has processed the command, then turn off the change
	//if stream = downstream and process = dev_handler then also update params bitmap
	if (process == DEV_HANDLER && stream == COMMAND) {
		//wait on pmap_sem
		my_sem_wait(pmap_sem, "pmap_sem@device_read");
	
		shm_ptr->pmap[0] &= (~(1 << dev_ix)); //turn off changed device bit in pmap[0]
		shm_ptr->pmap[dev_ix + 1] &= (~params_to_read); //turn off bits for params that were changed and then read in pmap[dev_ix + 1]
	
		//release pmap_sem
		my_sem_post(pmap_sem, "pmap_sem@device_read");
	}

	//release semaphore for appropriate stream and device
	if (stream == DATA) {
		my_sem_post(sems[dev_ix].data_sem, "data sem @device_read");
	} else {
		my_sem_post(sems[dev_ix].command_sem, "command sem @device_read");
	}
}

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
			shm_ptr->params[stream][dev_ix][i] = params[i];
		}
	}
	
	//turn on flag if executor is sending a command to the device
	//if stream = downstream and process = executor then also update params bitmap
	if (process == EXECUTOR && stream == COMMAND) {
		//wait on pmap_sem
		my_sem_wait(pmap_sem, "pmap_sem@device_write");
		
		shm_ptr->pmap[0] |= (1 << dev_ix); //turn on changed device bit in pmap[0]
		shm_ptr->pmap[dev_ix + 1] |= params_to_write; //turn on bits for params that were written in pmap[dev_ix + 1]
		
		//release pmap_sem
		my_sem_post(pmap_sem, "pmap_sem@device_write");
	}
	
	//release semaphore for appropriate stream and device
	if (stream == DATA) {
		my_sem_post(sems[dev_ix].data_sem, "data sem @device_write");
	} else {
		my_sem_post(sems[dev_ix].command_sem, "command sem @device_write");
	}
}

// ************************************ PUBLIC UTILITY FUNCTIONS ****************************************** //

void print_pmap ()
{
	uint32_t pmap[MAX_DEVICES + 1];
	get_param_bitmap(pmap);
	
	printf("changed devices: ");
	print_bitmap(MAX_DEVICES, pmap[0]);
	
	printf("changed params:\n");
	
	for (int i = 1; i < 33; i++) {
		if (pmap[0] & (1 << (i - 1))) {
			printf("\tdevice %d: ", i - 1);
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

void print_params (uint32_t params_to_print, param_val_t *params)
{
	for (int i = 0; i < MAX_PARAMS; i++) {
		if (params_to_print & (1 << i)) {
			printf("num = %d, p_i = %d, p_f = %f, p_b = %u\n", i, params[i].p_i, params[i].p_f, params[i].p_b);
		}
	}
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
void shm_init (process_t process)
{
	int fd_shm; //file descriptor of the memory-mapped shared memory
	char sname[SNAME_SIZE]; //for holding semaphore names
	
	if (process == DEV_HANDLER) {
		
		//mutual exclusion semaphore, catalog_mutex with initial value = 0
		if ((catalog_sem = sem_open(CATALOG_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED) {
			error("sem_open: catalog_mutex@dev_handler");
		}
		
		//mutual exclusion semaphore, pmap_mutex with initial value = 1
		if ((pmap_sem = sem_open(PMAP_MUTEX_NAME, O_CREAT, 0660, 1)) == SEM_FAILED) {
			error("sem_open: pmap_mutex@dev_handler");
		}
		
		//create shared memory block; initialize catalog and pmap to all zeros
		if ((fd_shm = shm_open(SHARED_MEM_NAME, O_RDWR | O_CREAT, 0660)) == -1) {
			error("shm_open: @dev_handler");
		}
		if (ftruncate(fd_shm, sizeof(shm_t)) == -1) {
			error("ftruncate: @dev_handler");
		}
		if ((shm_ptr = mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
			error("mmap: @dev_handler");
		}
		if (close(fd_shm) == -1) {
			error("close: @dev_handler");
		}
		shm_ptr->catalog = 0;
		for (int i = 0; i < MAX_DEVICES + 1; i++) {
			shm_ptr->pmap[i] = 0;
		}
		
		//create all the semaphores with initial value 1
		for (int i = 0; i < MAX_DEVICES; i++) {
			generate_sem_name(DATA, i, sname); //get the data name
			if ((sems[i].data_sem = sem_open((const char *) sname, O_CREAT, 0660, 1)) == SEM_FAILED) {
				error("sem_open: data sem@dev_handler");
			}
			generate_sem_name(COMMAND, i, sname); //get the command name
			if ((sems[i].command_sem = sem_open((const char *) sname, O_CREAT, 0660, 1)) == SEM_FAILED) {
				error("sem_open: command sem@dev_handler");
			}
		}
		
		//initialization complete; set catalog_mutex to 1 indicating shm segment available for client(s)
		my_sem_post(catalog_sem, "catalog_sem@dev_handler");
	} else {
		//mutual exclusion semaphore, catalog_mutex
		if ((catalog_sem = sem_open(CATALOG_MUTEX_NAME, 0, 0, 0)) == SEM_FAILED) {
			error("sem_open: catalog_mutex@client");
		}
		
		//mutual exclusion semaphore, pmap_mutex
		if ((pmap_sem = sem_open(PMAP_MUTEX_NAME, 0, 0, 0)) == SEM_FAILED) {
			error("sem_open: pmap_mutex@client");
		}
		
		//wait on catalog_sem to ensure shm has been created before opening
		my_sem_wait(catalog_sem, "catalog_mutex@client");
		
		//open shared memory block and map to client process virtual memory
		if ((fd_shm = shm_open(SHARED_MEM_NAME, O_RDWR, 0)) == -1) { //no O_CREAT
			error("shm_open: @client");
		}
		if ((shm_ptr = mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
			error("mmap: @client");
		}
		if (close(fd_shm) == -1) {
			error("close: @client");
		}
		
		//create all the semaphores with initial value 1
		for (int i = 0; i < MAX_DEVICES; i++) {
			generate_sem_name(DATA, i, sname); //get the data name
			if ((sems[i].data_sem = sem_open((const char *) sname, 0, 0, 0)) == SEM_FAILED) { //no O_CREAT
				error("sem_open: data sem@client");
			}
			generate_sem_name(COMMAND, i, sname); //get the command name
			if ((sems[i].command_sem = sem_open((const char *) sname, 0, 0, 0)) == SEM_FAILED) { //no O_CREAT
				error("sem_open: command sem@client");
			}
		}
		
		//release catalog_sem
		my_sem_post(catalog_sem, "catalog_mutex@client");
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
void shm_stop (process_t process)
{
	char sname[SNAME_SIZE]; //holding semaphore names
	
	//unmap the shared memory block
	if (munmap(shm_ptr, sizeof(shm_t)) == -1) {
		(process == DEV_HANDLER) ? error("munmap: @dev_handler") : error("munmap: @client");
	}
	
	//close all the semaphores
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (process == DEV_HANDLER) {
			my_sem_close(sems[i].data_sem, "data sem@dev_handler");
			my_sem_close(sems[i].command_sem, "command sem@dev_handler");
		} else {
			my_sem_close(sems[i].data_sem, "data sem@client");
			my_sem_close(sems[i].command_sem, "command sem@client");
		}
	}
	if (process == DEV_HANDLER) {
		my_sem_close(catalog_sem, "catalog sem@dev_handler");
		my_sem_close(pmap_sem, "pmap sem@dev_handler");
	} else {
		my_sem_close(catalog_sem, "catalog sem@client");
		my_sem_close(pmap_sem, "pmap sem@client");
	}
	
	//the device handler is also responsible for unlinking everything
	if (process == DEV_HANDLER) {
		//unlink shared memory block
		if (shm_unlink(SHARED_MEM_NAME) == -1) {
			error("shm_unlink");
		}
		
		//unlink semaphores
		for (int i = 0; i < MAX_DEVICES; i++) {
			generate_sem_name(DATA, i, sname);
			if (sem_unlink((const char *) sname) == -1) {
				error("sem_unlink: data sem");
			}
			generate_sem_name(COMMAND, i, sname);
			if (sem_unlink((const char *) sname) == -1) {
				error("sem_unlink: command sem");
			}
		}
		
		if (sem_unlink(CATALOG_MUTEX_NAME) == -1) {
			error("sem_unlink: catalog_sem");
		}
		if (sem_unlink(PMAP_MUTEX_NAME) == -1) {
			error("sem_unlink: pmap_sem");
		}
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
		if (!(shm_ptr->catalog & (1 << *dev_ix))) { //if the spot at dev_ix is free
			break;
		}
	}
	if (*dev_ix == MAX_DEVICES) {
		log_runtime(ERROR, "too many devices, connection unsuccessful");
		my_sem_post(catalog_sem, "catalog_sem"); //release the catalog semaphore
		return;
	}
	
	//wait on associated data and command sems
	my_sem_wait(sems[*dev_ix].data_sem, "data_sem");
	my_sem_wait(sems[*dev_ix].command_sem, "command_sem");
	
	//fill in dev_id for that device with provided values
	shm_ptr->dev_ids[*dev_ix].type = dev_type;
	shm_ptr->dev_ids[*dev_ix].year = dev_year;
	shm_ptr->dev_ids[*dev_ix].uid = dev_uid;
	
	//update the catalog
	shm_ptr->catalog |= (1 << *dev_ix);

	//reset param values to 0
	for (int i = 0; i < MAX_PARAMS; i++) {
		shm_ptr->params[DATA][*dev_ix][i] = (const param_val_t) {0};
		shm_ptr->params[COMMAND][*dev_ix][i] = (const param_val_t) {0};
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
	shm_ptr->catalog &= (~(1 << dev_ix));

	//reset param map values to 0
	shm_ptr->pmap[0] &= (~(1 << dev_ix));   //reset the changed bit flag in pmap[0]
	shm_ptr->pmap[dev_ix + 1] = 0;          //turn off all changed bits for the device
	
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
	if (!(shm_ptr->catalog & (1 << dev_ix))) {
		char msg[64];
		sprintf(msg, "no device at dev_ix = %d, read failed", dev_ix);
		log_runtime(DEBUG, msg);
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
	
	//check catalog and shm_ptr->dev_ids to determine if device exists
	for (int i = 0; i < MAX_DEVICES; i++) {
		if ((shm_ptr->catalog & (1 << i)) && (shm_ptr->dev_ids[i].uid == dev_uid)) {
			dev_ix = i;
			break;
		}
	}
	
	//if device doesn't exist, return immediately
	if (dev_ix == -1) {
		char msg[64];
		sprintf(msg, "no device at dev_uid = %llu, read failed", dev_uid);
		log_runtime(DEBUG, msg);
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
	if (!(shm_ptr->catalog & (1 << dev_ix))) {
		char msg[64];
		sprintf(msg, "no device at dev_ix = %d, write failed", dev_ix);
		log_runtime(DEBUG, msg);
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
	
	//check catalog and shm_ptr->dev_ids to determine if device exists
	for (int i = 0; i < MAX_DEVICES; i++) {
		if ((shm_ptr->catalog & (1 << i)) && (shm_ptr->dev_ids[i].uid == dev_uid)) {
			dev_ix = i;
			break;
		}
	}
	
	//if device doesn't exist, return immediately
	if (dev_ix == -1) {
		char msg[64];
		sprintf(msg, "no device at dev_uid = %llu, write failed", dev_uid);
		log_runtime(DEBUG, msg);
		return;
	}
	
	//call the helper to do the actual reading
	device_write_helper(dev_ix, process, stream, params_to_write, params);
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
		bitmap[i] = shm_ptr->pmap[i];
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
		dev_ids[i] = shm_ptr->dev_ids[i];
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
	
	*catalog = shm_ptr->catalog;
	
	//release catalog_sem
	my_sem_post(catalog_sem, "catalog_sem");
}
