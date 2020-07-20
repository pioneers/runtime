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

	virtual size_t DummyDevice::device_read (uint8_t param, uint8_t *data_buf);
	virtual size_t DummyDevice::device_write (uint8_t param, uint8_t *data_buf);
	virtual void DummyDevice::device_enable();
	virtual void DummyDevice::device_disable();
	virtual void DummyDevice::device_actions();

private:
	int16_t runtime; //param 0
	float shepherd;
	bool dawn;

	int16_t devops; //param 3
	float atlas;
	bool infra;

	int16_t sens; //param 6
	float pdb;
	bool mech;

	int16_t cpr; //param 9
	float edu;
	bool exec;

	int16_t pief; //param 12
	float funtime;
	bool sheep;

	int16_t dusk; //param 15
};

#endif