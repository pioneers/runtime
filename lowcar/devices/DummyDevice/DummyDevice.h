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

	virtual size_t device_read (uint8_t param, uint8_t *data_buf);
	virtual size_t device_write (uint8_t param, uint8_t *data_buf);
	virtual void device_enable();
	virtual void device_disable();
	virtual void device_actions();

private:
	int16_t runtime; //param 0
	float shepherd;
	uint8_t dawn;

	int16_t devops; //param 3
	float atlas;
	uint8_t infra;

	int16_t sens; //param 6
	float pdb;
	uint8_t mech;

	int16_t cpr; //param 9
	float edu;
	uint8_t exec;

	int16_t pief; //param 12
	float funtime;
	uint8_t sheep;

	int16_t dusk; //param 15
};

#endif
