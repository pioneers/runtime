#include <stdio.h>
#include <stdlib.h>
#include "../protobuf-c/dev_data.pb-c.h"

int main () 
{
	void *buf;                     // Buffer to store serialized data
	unsigned len;                  // Length of serialized data
	
	//initialize all the messages and submessages (let's send two devices, the first with 1 param and the second with 2 params)
	DevData dev_data = DEV_DATA__INIT;
	DevData__Device dev1 = DEV_DATA__DEVICE__INIT;
	DevData__Device dev2 = DEV_DATA__DEVICE__INIT;
	DevData__Param d1p1 = DEV_DATA__PARAM__INIT;
	DevData__Param d2p1 = DEV_DATA__PARAM__INIT;
	DevData__Param d2p2 = DEV_DATA__PARAM__INIT;					
	
	//set all the fields .....
	d1p1.name = "switch0";
	d1p1.val_case = DEV_DATA__PARAM__VAL_FVAL;
	d1p1.fval = 0.3;
	
	d2p1.name = "sensor0";
	d2p1.val_case = DEV_DATA__PARAM__VAL_IVAL;
	d2p1.ival = 42;
	
	d2p2.name = "bogus";
	d2p2.val_case = DEV_DATA__PARAM__VAL_BVAL;
	d2p2.bval = 1;
	
	dev1.type = "LimitSwitch";
	dev1.uid = "984789478297";
	dev1.itype = 12;
	dev1.n_params = 1;
	dev1.params = (DevData__Param **) malloc(dev1.n_params * sizeof(DevData__Param *));
	dev1.params[0] = &d1p1;
	
	dev2.type = "LineFollower";
	dev2.uid = "47834674267";
	dev2.itype = 13;
	dev2.n_params = 2;
	dev2.params = (DevData__Param **) malloc(dev2.n_params * sizeof(DevData__Param *));
	dev2.params[0] = &d2p1;
	dev2.params[1] = &d2p2;
	
	dev_data.n_devices = 2;
	dev_data.devices = (DevData__Device **) malloc(dev_data.n_devices * sizeof(DevData__Device *));
	dev_data.devices[0] = &dev1;
	dev_data.devices[1] = &dev2;
	//done setting all fields!
	
	len = dev_data__get_packed_size(&dev_data);
	
	buf = malloc(len);
	dev_data__pack(&dev_data, buf);
	
	fprintf(stderr, "Writing %d serialized bytes\n", len); // See the length of message
	fwrite(buf, len, 1, stdout); // Write to stdout to allow direct command line piping
	
	free(buf); // Free the allocated serialized buffer
	return 0;
}