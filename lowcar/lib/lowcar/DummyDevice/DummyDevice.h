#ifndef DUMMY_H
#define DUMMY_H

#include "Device.h"
#include "defs.h"

//DOES NOT REPRESENT ANY ACTUAL DEVICE
//Use to exercise the full extent of possible params and functionality of a lowcar device from runtime
//Flashable onto bare Arduino micro for testing

class DummyDevice : public Device
{
public:
	//construct a Dummy device
	DummyDevice();

	virtual uint8_t DummyDevice::device_read (uint8_t param, uint8_t *data_buf, size_t data_buf_len);
	virtual uint8_t DummyDevice::device_write (uint8_t param, uint8_t *data_buf);
	virtual void DummyDevice::device_enable();
	virtual void DummyDevice::device_disable();
	virtual void DummyDevice::device_actions();

private:
	int runtime; //param 0
	float shepherd;
	bool dawn;

	int devops; //param 3
	float atlas;
	bool infra;

	int sens; //param 6
	float pdb;
	bool mech;

	int cpr; //param 9
	float edu;
	bool exec;

	int pief; //param 12
	float funtime;
	bool sheep;

	int dusk; //param 15
};

#endif
