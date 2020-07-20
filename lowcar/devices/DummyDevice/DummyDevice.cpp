#include "DummyDevice.h"

typedef enum {
	RUNTIME = 0,
	SHEPHERD = 1,
	DAWN = 2,
	DEVOPS = 3,
	ATLAS = 4,
	INFRA = 5,
	SENS = 6,
	PDB = 7,
	MECH = 8,
	CPR = 9,
	EDU = 10,
	EXEC = 11,
	PIEF = 12,
	FUNTIME = 13,
	SHEEP = 14,
	DUSK = 15,
} param;

DummyDevice::DummyDevice () : Device (DeviceType::DUMMY_DEVICE, 13)
{
	this->runtime = 1;
	this->shepherd = 0.1;
	this->dawn = true;

	this->devops = 1;
	this->atlas = 0.1;
	this->infra = true;

	this->sens = 0;
	this->pdb = -0.1;
	this->mech = false;

	this->cpr = 0;
	this->edu = 0.0;
	this->exec = false;

	this->pief = 0;
	this->funtime = -0.1;
	this->sheep = false;

	this->dusk = 0;
}

//retrieves the appropriate instance variable, this whole function is a big lol
size_t DummyDevice::device_read (uint8_t param, uint8_t *data_buf)
{
	float	*float_buf	= (float *) data_buf;
	uint8_t *bool_buf	= (uint8_t *) data_buf;	// Convert bool to uint8_t to be consistent on C
	int16_t *int_buf	= (int16_t *) data_buf;

	switch (param) {

		case RUNTIME:
			int_buf[0] = this->runtime;
			return sizeof(int16_t);

		case SHEPHERD:
			float_buf[0] = this->shepherd;
			return sizeof(float);

		case DAWN:
			bool_buf[0] = (uint8_t) (this->dawn ? 1 : 0);
			return sizeof(uint8_t);

		case DEVOPS:
			int_buf[0] = this->devops;
			return sizeof(int16_t);

		case ATLAS:
			float_buf[0] = this->atlas;
			return sizeof(float);

		case INFRA:
			bool_buf[0] = (uint8_t) (this->infra ? 1 : 0);
			return sizeof(uint8_t);

		case SENS:
			break;

		case PDB:
			break;

		case MECH:
			break;

		case CPR:
			break;

		case EDU:
			break;

		case EXEC:
			break;

		case PIEF:
			int_buf[0] = this->pief;
			return sizeof(int16_t);

		case FUNTIME:
			float_buf[0] = this->funtime;
			return sizeof(float);

		case SHEEP:
			bool_buf[0] = (uint8_t) (this->sheep ? 1 : 0);
			return sizeof(uint8_t);

		case DUSK:
			int_buf[0] = this->dusk;
			return sizeof(int16_t);
	}
}

//writes the appropriate instance variable; this whole function is also a big lol
size_t DummyDevice::device_write (uint8_t param, uint8_t *data_buf)
{
	switch (param) {

		case RUNTIME:
			break;

		case SHEPHERD:
			break;

		case DAWN:
			break;

		case DEVOPS:
			break;

		case ATLAS:
			break;

		case INFRA:
			break;

		case SENS:
			this->sens = ((int16_t *) data_buf)[0];
			return sizeof(int16_t);

		case PDB:
			this->pdb = ((float *) data_buf)[0];
			return sizeof(float);

		case MECH:
			this->mech = ((bool *) data_buf)[0];
			return sizeof(uint8_t);

		case CPR:
			this->cpr = ((int16_t *) data_buf)[0];
			return sizeof(int16_t);

		case EDU:
			this->edu = ((float *) data_buf)[0];
			return sizeof(float);

		case EXEC:
			this->exec = ((bool *) data_buf)[0];
			return sizeof(uint8_t);

		case PIEF:
			this->pief = ((int16_t *) data_buf)[0];
			return sizeof(int16_t);

		case FUNTIME:
			this->funtime = ((float *) data_buf)[0];
			return sizeof(float);

		case SHEEP:
			this->sheep = ((bool *) data_buf)[0];
			return sizeof(uint8_t);

		case DUSK:
			this->dusk = ((int16_t *) data_buf)[0];
			return sizeof(int16_t);
	}
}

void DummyDevice::device_enable()
{
	this->msngr->lowcar_printf("DUMMY DEVICE ENABLED");
}

void DummyDevice::device_disable()
{
	this->msngr->lowcar_printf("DUMMY DEVICE DISABLED");
}

void DummyDevice::device_actions()
{
	static uint64_t last_update_time = 0;
	// Simulate read-only params changing

	if (millis() - last_update_time > 500) {

		this->runtime += 2;
		this->shepherd += 1.9;
		this->dawn = !this->dawn;

		this->devops++;
		this->atlas += 0.9;
		this->infra = true;
		
		//this->msngr->lowcar_printf("funtime = %f", this->funtime);
		//this->msngr->lowcar_printf("atlas = %f", this->atlas);
		//this->msngr->lowcar_printf("pdb = %f", this->pdb);

		last_update_time = millis();
	}

}
