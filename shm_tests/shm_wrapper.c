#include "shm_wrapper.h"

#define PRM_SNAME "/prm_name" //name of param bitmap semaphore
#define SHM_PMAP_KEY 0x87654321 //use this key for the param bitmap

#define CONN_SNAME "/conn_name" //name of device connection semaphore
#define SHM_CATALOG_KEY 0x12345678 //use this key for the catalog

#define SNAME_SIZE 32 //size of buffers that hold semaphore names, in bytes

#define DEVICE_ID 2 //array index into the dev_shmids array to match the already defined UPSTREAM 0 and DOWNSTREAM 1

// ***************************************** PRIVATE TYPEDEFS ********************************************* //

typedef struct pmap {
	uint16_t maps[MAX_PARAMS + 1]; //17 16-bit bitmaps for the changed parameters of the devices
} prm_map_t;

typedef struct catalog {
	uint16_t valid_dev_bitmap; //valid devices bitmap
	key_t keys[3][MAX_DEVICES]; //all keys for upstream (keys[0]), downstream (keys[1]) dev_data_t's, and dev_id (keys[2])
} catalog_t;

typedef struct dev_data {
	param_t params[MAX_PARAMS]; //maximum number of params is 16
} dev_data_t;

typedef struct dual_sem {
	sem_t *up_sem;
	sem_t *down_sem;
} dual_sem_t;

// *********************************** WRAPPER-SPECIFIC GLOBAL VARS **************************************** //

sem_t *prm_map_sem; //semaphore on updated params bitmap
prm_map_t *prm_map; //struct mapped to updated params bitmap (shm)
int prm_map_shmid; //shm id of the params bitmap (only used by device handler)

sem_t *dev_conn_sem; //semaphore on all data involved in device connect, disconnect, register i.e. catalog, dev_id
catalog_t *catalog; //struct mapped to catalog (shm)
int catalog_shmid; //shm id of the catalog (only used by device handler)
uint16_t catalog_local; //local catalog for other processes to compare to shm catalog

dev_data_t *upstream[MAX_DEVICES]; //array of pointers to upstream info for all devices (shm)
dev_data_t *downstream[MAX_DEVICES]; //array of pointers to downstream info for all devices (shm)
dev_id_t *dev_id[MAX_DEVICES]; //array of pointers to device identification info for all devices (shm)
dual_sem_t *sems[MAX_DEVICES]; //array of semaphores for the upstream and downstream dev_infos
int dev_shmids[3][MAX_DEVICES]; //shm ids of the upstream, downstream, and device id infos (only used by device handler)

// ******************************************** HELPER FUNCTIONS ****************************************** //

static void generate_sem_name (uint8_t stream, int dev_ix, char *name)
{
	if (stream == UPSTREAM) {
		sprintf(name, "/up_sem_%d", dev_ix);
	} else {
		sprintf(name, "/down_sem_%d", dev_ix);
	}
}

//sets up a device at dev_ix, puts the tmp_shmids and snames into the pointer arguments
static int device_attach (int dev_ix, int *tmp_shmid_up, int *tmp_shmid_down, int *tmp_shmid_dev_id, char *sname_up, char *sname_down)
{
	//get and attach the upstream and downstream device data shared memory blocks
	*tmp_shmid_up = shmget(catalog->keys[UPSTREAM][dev_ix], sizeof(dev_data_t), 0666 | IPC_CREAT);
	*tmp_shmid_down = shmget(catalog->keys[DOWNSTREAM][dev_ix], sizeof(dev_data_t), 0666 | IPC_CREAT);
	if (*tmp_shmid_up == -1 || *tmp_shmid_down == -1) {
		perror("dev data create");
		return 1;
	}
	upstream[dev_ix] = shmat(*tmp_shmid_up, NULL, 0);
	downstream[dev_ix] = shmat(*tmp_shmid_down, NULL, 0);
	if ((upstream[dev_ix] == (void *) -1) || (downstream[dev_ix] == (void *) -1)) {
		perror("dev data attach");
		return 2;
	}
	
	//open two named semaphores and store them in sems[dev_ix]
	//memory freed in device_update_registry or shm_stop
	sems[dev_ix] = (dual_sem_t *) malloc(sizeof(dual_sem_t) * 1);
	generate_sem_name(UPSTREAM, dev_ix, sname_up);
	generate_sem_name(DOWNSTREAM, dev_ix, sname_down);
	sems[dev_ix]->up_sem = sem_open((const char *) sname_up, O_CREAT, 0666, 1);
	sems[dev_ix]->down_sem = sem_open((const char *) sname_down, O_CREAT, 0666, 1);
	
	//get and attach device id shared memory block
	*tmp_shmid_dev_id = shmget(catalog->keys[DEVICE_ID][dev_ix], sizeof(dev_id_t), 0666 | IPC_CREAT);
	if (*tmp_shmid_dev_id == -1) {
		perror("dev id create");
		return 3;
	}
	dev_id[dev_ix] = shmat(*tmp_shmid_dev_id, NULL, 0);
	if (dev_id[dev_ix] == (void *) -1) {
		perror("dev id attach");
		return 4;
	}
	
	return 0;
}

//unmaps the shared memory associated with a device at dev_ix
static int device_detach (int dev_ix) {
	//close mappings for all three shared memory slots
	if (shmdt(upstream[dev_ix]) == -1) {
		perror("upstream detach");
		return 1;
	}
	if (shmdt(downstream[dev_ix]) == -1) {
		perror("downstream detach");
		return 2;
	}
	if (shmdt(dev_id[dev_ix]) == -1) {
		perror("dev_id detach");
		return 3;
	}
	
	return 0;
}

// ******************************************** PUBLIC FUNCTIONS ****************************************** //

/*
Call this function from every process that wants to use the shared memory wrapper
Should be called before any other action happens
The device handler process is responsible for initializing the catalog and updated param bitmap
	- process: one of DEV_HANDLER, EXECUTOR, NET_HANDLER to specify which process this function is being
		called from
Returns 0 on success, various numbers from 1 - 4 on various errors (prints to stderr)
*/
int shm_init (uint8_t process)
{
	int tmp_shmid;
		
	//get and attach param bitmap; open semaphore on that bitmap
	tmp_shmid = shmget(SHM_PMAP_KEY, sizeof(prm_map_t), 0666 | IPC_CREAT);
	if (tmp_shmid == -1) {
		perror("param map create");
		return 1;
	}
	prm_map = shmat(tmp_shmid, NULL, 0);
	if (prm_map == (void *) -1) {
		perror("param map attach");
		return 2;
	}
	prm_map_sem = sem_open(PRM_SNAME, O_CREAT, 0666, 1);
	//save the shmid if it's the device handler, and zero out the param bitmap
	if (process == DEV_HANDLER) {
		prm_map_shmid = tmp_shmid;
		for (int i = 0; i < MAX_DEVICES + 1; i++) {
			prm_map->maps[i] = 0;
		}
	}
	
	//get and attach catalog; open semaphore on the catalog
	tmp_shmid = shmget(SHM_CATALOG_KEY, sizeof(catalog_t), 0666 | IPC_CREAT);
	if (tmp_shmid == -1) {
		perror("catalog create");
		return 3;
	}
	catalog = shmat(tmp_shmid, NULL, 0);
	if (catalog == (void *) -1) {
		perror("catalog attach");
		return 4;
	}
	dev_conn_sem = sem_open(CONN_SNAME, O_CREAT, 0666, 1);
	//save the shmid if it's the device handler, and initialize catalog bitmap
	if (process == DEV_HANDLER) {
		catalog_shmid = tmp_shmid;
		catalog->valid_dev_bitmap = 0;
		for (int i = 0; i < MAX_DEVICES; i++) {
			catalog->keys[UPSTREAM][i] = i * 4096;
			catalog->keys[DOWNSTREAM][i] = i * 4096 + 1024;
			catalog->keys[DEVICE_ID][i] = i * 4096 + 2048;
		}
	} else { //otherwise set local copy of catalog to 0
		catalog_local = 0;
	}
	
	return 0;
}

/*
Call this function if process no longer wishes to connect to shared memory wrapper
No other actions will work after this function is called
Device handler is responsible for marking shared memory for destruction after all detach
	- process: one of DEV_HANDLER, EXECUTOR, NET_HANDLER to specify which process this function is being
		called from
Returns 0 on success, various numbers from 1 - 7 on various errors (prints to stderr)
*/
int shm_stop (uint8_t process)
{
	int ret;
	//disconnect all remaining devices
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (catalog_local & (1 << i)) { //if device has mappings on this process
			//detach the associated shared memory blocks
			ret = device_detach(i);
			if (ret != 0) { //if errored
				return ret + 4; //+4 since other functions return 1 - 4 already
			}
			
			//close the semaphores (it will have been unlinked by device handler) and free memory
			sem_close(sems[i]->up_sem);
			sem_close(sems[i]->down_sem);
			free(sems[i]);
		}
	}
	
	//detach the param bitmap; set shm for removal if device handler, close the semaphore
	if (shmdt(prm_map) == -1) {
		perror("param bitmap detach");
		return 1;
	}
	if (process == DEV_HANDLER) {
		if (shmctl(prm_map_shmid, IPC_RMID, 0) == -1) {
			perror("param bitmap removal");
			return 3;
		}
	}
	sem_close(prm_map_sem);
	
	//detach the catalog, set shm for removal if device handler, close the semaphore
	if (shmdt(catalog) == -1) {
		perror("catalog detach");
		return 2;
	}
	if (process == DEV_HANDLER) {
		if (shmctl(catalog_shmid, IPC_RMID, 0) == -1) {
			perror("catalog removal");
			return 4;
		}
	}
	sem_close(dev_conn_sem);
	
	return 0;
}

/*
Should only be called from device handler
Finds a place to put the device data shared memory blocks and sets it up
	- dev_type: the type of the device being connected e.g. LIMITSWITCH, LINEFOLLOWER, etc; see device_config.h
	- dev_uid: the unique, random 64-bit uid assigned to the device when flashing
Returns 0 on success, various numbers from 1 - 4 for various errors (prints to stderr)
*/
int device_connect (uint16_t dev_type, uint64_t dev_uid)
{
	int dev_ix, tmp_shmid_up, tmp_shmid_down, tmp_shmid_dev_id, ret;
	char sname_up[SNAME_SIZE];
	char sname_down[SNAME_SIZE];
	
	//lock device connection semaphore
	sem_wait(dev_conn_sem);
	
	//find the next open spot for a device
	for (dev_ix = 0; dev_ix < MAX_DEVICES; dev_ix++) {
		if (!((1 << dev_ix) & catalog->valid_dev_bitmap)) { //if catalog->valid_dev_bitmap is free in that spot
			break;
		}
	}
	
	//attach the device (gets and opens associated shared memory blocks)
	ret = device_attach(dev_ix, &tmp_shmid_up, &tmp_shmid_down, &tmp_shmid_dev_id, sname_up, sname_down);
	if (ret != 0) { //if errored
		return ret;
	}
	
	//fill device id shared memory block
	dev_id[dev_ix]->type = dev_type;
	dev_id[dev_ix]->uid = dev_uid;
	
	//store the three shmids in dev_shmids
	dev_shmids[UPSTREAM][dev_ix] = tmp_shmid_up;
	dev_shmids[DOWNSTREAM][dev_ix] = tmp_shmid_down;
	dev_shmids[DEVICE_ID][dev_ix] = tmp_shmid_dev_id;
	
	//update the catalog bitmap
	catalog->valid_dev_bitmap |= (1 << dev_ix);
	
	//release device connection semaphore
	sem_post(dev_conn_sem);
	
	return 0;
}

/*
Should only be called from device handler
Disconnects a device with a given index by closing mappings to all associated shared memory
and unlinking associated semaphores. Also updates the catalog bitmap
	- dev_ix: index of device in catalog to be disconnected
Returns 0 on success, various numbers from 1 - 6 for various errors (prints to stderr)
*/
int device_disconnect (int dev_ix)
{
	int ret;
	char sname_up[SNAME_SIZE];
	char sname_down[SNAME_SIZE];
	
	//lock device connection semaphore
	sem_wait(dev_conn_sem);
	
	//update the catalog bitmap
	catalog->valid_dev_bitmap &= (~(1 << dev_ix)); //turn off the bit in that location
	
	//detach the shared memory associated with this device
	ret = device_detach(dev_ix);
	if (ret != 0) { //if errored
		return ret;
	}
	
	//mark these segments to be destroyed once all processes detach from it
	if (shmctl(dev_shmids[UPSTREAM][dev_ix], IPC_RMID, 0) == -1) {
		perror("upstream removal");
		return 4;
	}
	if (shmctl(dev_shmids[DOWNSTREAM][dev_ix], IPC_RMID, 0) == -1) {
		perror("downstream removal");
		return 5;
	}
	if (shmctl(dev_shmids[DEVICE_ID][dev_ix], IPC_RMID, 0) == -1) {
		perror("dev_id removal");
		return 6;
	}
	
	//unlink both device semaphores
	generate_sem_name(UPSTREAM, dev_ix, sname_up);
	generate_sem_name(DOWNSTREAM, dev_ix, sname_down);
	sem_unlink((const char *) sname_up);
	sem_unlink((const char *) sname_down);
	
	//release device connection semaphore
	sem_post(dev_conn_sem);
	
	return 0;
}

/*
Should be called periodically by every process except the device handler
Compares local and shared memory copy of the valid device indices and determines if any devices have
recently connected / disconnected. If so, then takes appropriate action to update device registry
for this process. Updates local catalog.
Returns 0 on success, various numbers from 1 - 7 for various errors (prints to stderr)
*/
int device_update_registry ()
{
	int dev_ix, tmp_shmid_up, tmp_shmid_down, tmp_shmid_dev_id, ret;
	char sname_up[SNAME_SIZE];
	char sname_down[SNAME_SIZE];
	
	//lock device connection sempahore
	sem_wait(dev_conn_sem);
	
	//if nothing to do, release the connection semaphore and return
	if (catalog_local == catalog->valid_dev_bitmap) {
		sem_post(dev_conn_sem);
		return 0;
	}
	
	for (int i = 0; i < MAX_DEVICES; i++) {
		//if we have a new device in the shm catalog, we need to set up that device
		if (!((1 << i) & catalog_local) && ((1 << i) & catalog->valid_dev_bitmap)) {
			//get and attach the associated shared memory blocks
			ret = device_attach(dev_ix, &tmp_shmid_up, &tmp_shmid_down, &tmp_shmid_dev_id, sname_up, sname_down);
			if (ret != 0) { //if errored
				return ret;
			}
			
			//update the local bitmap
			catalog_local |= (1 << i);
			
			//else if we have a device that's missing in the shm catalog, we need to remove that device
		} else if (((1 << i) & catalog_local) && !((1 << i) & catalog->valid_dev_bitmap)) {
			//detach the associated shared memory blocks
			ret = device_detach(dev_ix);
			if (ret != 0) { //if errored
				return ret + 4; //+4 since other functions return 1 - 4 already
			}
			
			//close the semaphores (it will have been unlinked by device handler) and free memory
			sem_close(sems[i]->up_sem);
			sem_close(sems[i]->down_sem);
			free(sems[i]);
			
			//update the local bitmap
			catalog_local &= (~(1 << i));
		}
	}
	
	//release device connection semaphore
	sem_post(dev_conn_sem);
	
	return 0;
}

/*	
Should be called from every process wanting to read the device data
Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
Grabs either one or two semaphores depending on calling process and stream requested.
	- dev_ix: device index of the device whose data is being requested
	- process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
	- stream: the requested block to read from, one of UPSTREAM, DOWNSTREAM
	- params_to_read: bitmap representing which params to be read 
		(nonexistent params should have corresponding bits set to 0)
	- params: pointer to array of param_t's that is at least as long as highest requested param number
		device data will be read into the corresponding param_t's
No return value.
*/
void device_read (int dev_ix, uint8_t process, uint8_t stream, uint16_t params_to_read, param_t *params)
{
	//only wait for the param bitmap semaphore if reading downstream block from device handler
	if (process == DEV_HANDLER && stream == DOWNSTREAM) {
		sem_wait(prm_map_sem);
	}
	
	//lock associated semaphore depending on which stream was requested
	if (stream == UPSTREAM) {
		sem_wait(sems[dev_ix]->up_sem);
	} else if (stream == DOWNSTREAM) {
		sem_wait(sems[dev_ix]->down_sem);
	}
	
	//loop through param bits and pull out requested params into provided pointer
	for (int i = 0; i < 16; i++) {
		if (params_to_read & (1 << i)) {
			params[i].num = i;
			
			if (stream == UPSTREAM) {
				params[i].p_i = upstream[dev_ix]->params[i].p_i;
				params[i].p_f = upstream[dev_ix]->params[i].p_f;
				params[i].p_b = upstream[dev_ix]->params[i].p_b;
			} else if (stream == DOWNSTREAM) {
				params[i].p_i = downstream[dev_ix]->params[i].p_i;
				params[i].p_f = downstream[dev_ix]->params[i].p_f;
				params[i].p_b = downstream[dev_ix]->params[i].p_b;

				//only on downstream block from device handler do we zero out corresponding bits
				if (process == DEV_HANDLER) {
					prm_map->maps[dev_ix + 1] &= (~(1 << i));
				}
			}
		}
	}
	
	//only on downstream block from device handler do we zero out the corresponding device bit
	if (process == DEV_HANDLER && prm_map->maps[dev_ix + 1] == 0) {
		prm_map->maps[0] &= (~(1 << dev_ix));
	}
	
	//release associated semaphore depending on which stream was requested
	if (stream == UPSTREAM) {
		sem_post(sems[dev_ix]->up_sem);
	} else if (stream == DOWNSTREAM) {
		sem_post(sems[dev_ix]->down_sem);
	}
	
	//only release the param bitmap semaphore again if reading downstream block from device handler
	if (process == DEV_HANDLER && stream == DOWNSTREAM) {
		sem_post(prm_map_sem);
	}
}

/*	
Should be called from every process wanting to write to the device data
Takes care of updating the param bitmap for fast transfer of commands from executor to device handler
Grabs either one or two semaphores depending on calling process and stream requested.
	- dev_ix: device index of the device whose data is being written
	- process: the calling process, one of DEV_HANDLER, EXECUTOR, or NET_HANDLER
	- stream: the requested block to write to, one of UPSTREAM, DOWNSTREAM
	- params_to_read: bitmap representing which params to be written
		(nonexistent params should have corresponding bits set to 0)
	- params: pointer to array of param_t's that is at least as long as highest requested param number
		device data will be written into the corresponding param_t's
No return value.
*/
void device_write (int dev_ix, uint8_t process, uint8_t stream, uint16_t params_to_write, param_t *params)
{
	//only wait for the param bitmap semaphore if reading downstream block from device handler
	if (process == DEV_HANDLER && stream == DOWNSTREAM) {
		sem_wait(prm_map_sem);
	}
	
	//lock associated semaphore depending on which stream was requested
	if (stream == UPSTREAM) {
		sem_wait(sems[dev_ix]->up_sem);
	} else if (stream == DOWNSTREAM) {
		sem_wait(sems[dev_ix]->down_sem);
	}
	
	//loop through param bits and write in requested params into the shared memory
	for (int i = 0; i < 16; i++) {
		if (params_to_write & (1 << i)) {
			params[i].num = i;
			
			if (stream == UPSTREAM) {
				upstream[dev_ix]->params[i].p_i = params[i].p_i;
				upstream[dev_ix]->params[i].p_f = params[i].p_f;
				upstream[dev_ix]->params[i].p_b = params[i].p_b;
			} else if (stream == DOWNSTREAM) {
				downstream[dev_ix]->params[i].p_i = params[i].p_i;
				downstream[dev_ix]->params[i].p_f = params[i].p_f;
				downstream[dev_ix]->params[i].p_b = params[i].p_b;

				//only on downstream block from device handler do we turn on the corresponding bits
				if (process == DEV_HANDLER) {
					prm_map->maps[dev_ix + 1] |= (1 << i);
				}
			}
		}
	}
	
	//only on downstream block from device handler do we turn on the corresponding device bit
	if (process == DEV_HANDLER) {
		prm_map->maps[0] |= (1 << dev_ix);
	}
	
	//release associated semaphore depending on which stream was requested
	if (stream == UPSTREAM) {
		sem_post(sems[dev_ix]->up_sem);
	} else if (stream == DOWNSTREAM) {
		sem_post(sems[dev_ix]->down_sem);
	}
	
	//only release the param bitmap semaphore again if reading downstream block from device handler
	if (process == DEV_HANDLER && stream == DOWNSTREAM) {
		sem_post(prm_map_sem);
	}
}

/*
Should be called from all processes that want to know current state of the param bitmap (i.e. device handler)
Blocks on the param bitmap semaphore for obvious reasons
	- bitmap: pointer to array of 17 16-bit integers to copy the bitmap into
No return value.
*/
void get_param_bitmap (uint16_t *bitmap)
{
	//lock the param bitmap semaphore
	sem_wait(prm_map_sem);
	
	//copy all the maps out of the shm bitmap into the provided array
	for (int i = 0; i < (MAX_PARAMS + 1); i++) {
		bitmap[i] = prm_map->maps[i];
	}
	
	//release the param bitmap semaphore
	sem_post(prm_map_sem);
}

/*
Should be called from all processes that want to know device identifiers of all currently connected devices
Blocks on device connection semaphore
	- dev_ids: pointer to array of dev_id_t's to copy the information into
No return value.
*/
void get_device_identifiers (dev_id_t *dev_ids)
{
	//lock the device connection semaphore
	sem_wait(dev_conn_sem);
	
	//copy all valid device information into the provided array
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (catalog->valid_dev_bitmap & (1 << i)) { //if valid device here
			dev_ids[i].type = dev_id[i]->type;
			dev_ids[i].uid = dev_id[i]->uid;
		}
	}
	
	//release the device connection semaphore
	sem_post(dev_conn_sem);
}