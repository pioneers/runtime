#include <stdio.h>
#include <stdlib.h>
#include "../pbc_gen/device.pb-c.h"

int main () 
{
	void *buf;                     // Buffer to store serialized data
	unsigned len;                  // Length of serialized data
	
	//initialize all the messages and submessages (let's send two devices, the first with 1 param and the second with 2 params)
	DevData dev_data = DEV_DATA__INIT;
	Device dev1 = DEVICE__INIT;
	Device dev2 = DEVICE__INIT;
	Param d1p1 = PARAM__INIT;
	Param d2p1 = PARAM__INIT;
	Param d2p2 = PARAM__INIT;					
	
	//set all the fields .....
	d1p1.name = "switch0";
	d1p1.val_case = PARAM__VAL_FVAL;
	d1p1.fval = 0.3;
	
	d2p1.name = "sensor0";
	d2p1.val_case = PARAM__VAL_IVAL;
	d2p1.ival = 42;
	
	d2p2.name = "bogus";
	d2p2.val_case = PARAM__VAL_BVAL;
	d2p2.bval = 1;
	
	dev1.name = "LimitSwitch";
	dev1.uid = 984789478297;
	dev1.type = 12;
	dev1.n_params = 1;
	dev1.params = (Param **) malloc(dev1.n_params * sizeof(Param *));
	dev1.params[0] = &d1p1;
	
	dev2.name = "LineFollower";
	dev2.uid = 47834674267;
	dev2.type = 13;
	dev2.n_params = 2;
	dev2.params = (Param **) malloc(dev2.n_params * sizeof(Param *));
	dev2.params[0] = &d2p1;
	dev2.params[1] = &d2p2;
	
	dev_data.n_devices = 2;
	dev_data.devices = (Device **) malloc(dev_data.n_devices * sizeof(Device *));
	dev_data.devices[0] = &dev1;
	dev_data.devices[1] = &dev2;
	//done setting all fields!
	
	len = dev_data__get_packed_size(&dev_data);
	
	buf = malloc(len);
	dev_data__pack(&dev_data, buf);
	
	fprintf(stderr, "Writing %d serialized bytes\n", len); // See the length of message
	fwrite(buf, len, 1, stdout); // Write to stdout to allow direct command line piping
	
	free(buf); // Free the allocated serialized buffer
	free(dev1.params);
	free(dev2.params);
	free(dev_data.devices);
	return 0;
}