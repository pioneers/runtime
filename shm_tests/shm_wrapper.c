#include "shm_wrapper.h"

//info about a device
typedef struct dev {
	int dev_ix; //index into the devs array (speed up lookup)
	uint16_t dev_type; //LIMITSWITCH, KOALABEAR, etc
	uint64_t uid; //unique identifier
	param_t params[16]; //all of the parameters associated with this device; position in the array corresponds to param number
} dev_t;

//holds device info in shared memory
typedef struct dev_shm {
	int up_id;
	int down_id;
	dev_t *up_info;
	dev_t *down_info;
	sem_t up_sem;
	sem_t down_sem;
} dev_shm_t;

//catalog for which device indices are being used go in here
typedef struct catalog {
	uint16_t bitmap; //0 in invalid spot; 1 in valid spot
	sem_t *cat_sem; //catalog sempahore
} catalog_t;

//a device will have the same index into these arrays
dev_t **devs; //devices
dev_shm_t **dev_shms; //contains shm and semaphore info

//****************************************************** HELPER FUNCTIONS ***************************************//

//****************************************************** PUBLIC FUNCTIONS ***************************************//

void shm_init ()
{
	devs = (dev_t **) malloc((sizeof(dev_t *)) * MAX_DEVICES); //get MAX_DEVICES dev_t pointers
	dev_shms = (dev_shm_t **) malloc((sizeof(dev_shm_t *)) * MAX_DEVICES); //get MAX_DEVICES dev_shm_t pointers
	
	//initialize every element in both arrays to 0
	for (int i = 0; i < MAX_DEVICES; i++) {
		devs + i = NULL;
		dev_shms + i = NULL;
	}
}

void shm_close ()
{
	//free any remaining devices and associated dev_shms structs
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (devs[i] != NULL) {
			free(devs + i);
		}
		if (dev_shms[i] != NULL) {
			free(dev_shms + i);
		}
	}
	
	//free the arrays themselves
	free(devs);
	free(dev_shms);
}

//return 0 on success, 1 on failure
int device_create (uint16_t dev_type, uint64_t uid); //will return the dev_ix for new device, or -1 on failure
{
	int dev_ix;
	dev_shm_t *new_dev_shm;
	
	//find a spot in the two arrays to store the device and fill it in
	for (dev_ix = 0; dev_ix < MAX_DEVICES; dev_ix++) {
		if (devs[dev_ix] == NULL) {
			break;
		}
	}
	if (dev_ix == MAX_DEVICES) { //if no space then return
		return -1;
	}
	
	//look up what device it is <-- use the device config, whatever that ends up looking like TODO
	
	
	//get and initialize a new dev_shm_t structure
	new_dev_shm = (dev_shm_t *) malloc (sizeof(dev_shm_t) * 1);
	new_dev_shm->up_id = shmget((key_t) (dev_ix * 2), sizeof(struct dev_t), 0666 | IPC_CREAT); //use the dev_ix * 2 and dev_ix * 2 + 1 as keys
	new_dev_shm->down_id = shmget((key_t) (dev_ix * 2 + 1), sizeof(struct dev_t), 0666 | IPC_CREAT);
	new_dev_shm->up_info = shmat(new_dev_shm->up_id, NULL, 0);
	new_dev_shm->down_info = shmat(new_dev_shm->down_id, NULL, 0);
	
	if (sem_init(&(new_dev_shm->up_sem), 1, 0) == -1) {
		printf("init upstream semaphore error\n");
		perror();
		return 1;
	}
	if (sem_init(&(new_dev_shm->down_sem), 1, 0) == -1) {
		printf("init downstream semaphore error\n");
		perror();
		return 1;
	}
	
	
	 	
	new_dev->dev_ix = dev_ix;
	new_dev->dev_type = dev_type;
	new_dev->uid = uid;
	
	//create two new keys
	//create shared memory blocks
	//attach to them
	//init two semaphores
	//malloc a struct 
	//put the keys and semaphores in it
	//put the newly created dev_shm struct into the array
	
	return dev_ix;
}

void device_write (int dev_ix, uint16_t params_to_write, param_t *params)
{
	//figure out which params are being written
	//obtain corresponding downstream id semaphore
	//write params to those locations
	//release the semaphore
	//set the bitmaps to notify devices
}

void device_read (int dev_ix, uint16_t params_to_read, param_t *params)
{
	//figure out which params have been requested
	//obtain corresponding upstream id semaphore
	//read params out into the provided array
	//release the semaphore
}

void device_close (int dev_ix)
{
	//remove from arrays
	//free associated dev_shm_t
}

//return value = number of devices with changed params
//pass in an array of uint16_t's that is 17 elements long
//index 0 will be a bitmap of which devices have changed params
//indices 1 - 16 will be a bitmap of which params for those devices have changed
//for those params that have changed, retrieve updated value by calling device_read()
int get_updated_params (uint16_t *update_info)
{
	//basically just return the bitmap
	//or perhaps an array of bitmaps, the first corresponding to which devices have been written
	//and then each one in succession corresponding to the params in each device
}