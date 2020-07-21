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
	this->dawn = TRUE;

	this->devops = 1;
	this->atlas = 0.1;
	this->infra = TRUE;

	this->sens = 0;
	this->pdb = -0.1;
	this->mech = FALSE;

	this->cpr = 0;
	this->edu = 0.0;
	this->exec = FALSE;

	this->pief = 0;
	this->funtime = -0.1;
	this->sheep = FALSE;

	this->dusk = 0;
}

//retrieves the appropriate instance variable, this whole function is a big lol
size_t DummyDevice::device_read (uint8_t param, uint8_t *data_buf)
{
	float	*float_buf	= (float *) data_buf;
	int16_t *int_buf	= (int16_t *) data_buf;

	switch (param) {

		case RUNTIME:
			int_buf[0] = this->runtime;
			return sizeof(this->runtime);

		case SHEPHERD:
			float_buf[0] = this->shepherd;
			return sizeof(this->shepherd);

		case DAWN:
			data_buf[0] = this->dawn;
			return sizeof(this->dawn);

		case DEVOPS:
			int_buf[0] = this->devops;
			return sizeof(this->devops);

		case ATLAS:
			float_buf[0] = this->atlas;
			return sizeof(this->atlas);

		case INFRA:
			data_buf[0] = this->infra;
			return sizeof(this->infra);

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
			return sizeof(this->pief);

		case FUNTIME:
			float_buf[0] = this->funtime;
			return sizeof(this->funtime);

		case SHEEP:
			data_buf[0] = this->sheep;
			return sizeof(this->sheep);

		case DUSK:
			int_buf[0] = this->dusk;
			return sizeof(this->dusk);
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
			return sizeof(this->sens);

		case PDB:
			this->pdb = ((float *) data_buf)[0];
			return sizeof(this->pdb);

		case MECH:
			this->mech = data_buf[0];
			return sizeof(this->mech);

		case CPR:
			this->cpr = ((int16_t *) data_buf)[0];
			return sizeof(this->cpr);

		case EDU:
			this->edu = ((float *) data_buf)[0];
			return sizeof(this->edu);

		case EXEC:
			this->exec = data_buf[0];
			return sizeof(this->exec);

		case PIEF:
			this->pief = ((int16_t *) data_buf)[0];
			return sizeof(this->pief);

		case FUNTIME:
			this->funtime = ((float *) data_buf)[0];
			return sizeof(this->funtime);

		case SHEEP:
			this->sheep = data_buf[0];
			return sizeof(this->sheep);

		case DUSK:
			this->dusk = ((int16_t *) data_buf)[0];
			return sizeof(this->dusk);
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
	//static uint64_t last_count_time = 0;
	uint64_t curr = millis();
	
	// Simulate read-only params changing
	if (curr - last_update_time > 500) {

		this->runtime += 2;
		this->shepherd += 1.9;
		this->dawn = !this->dawn;

		this->devops++;
		this->atlas += 0.9;
		this->infra = TRUE;

		//this->msngr->lowcar_print_float("funtime", this->funtime);
		//this->msngr->lowcar_print_float("atlas", this->atlas);
		//this->msngr->lowcar_print_float("pdb", this->pdb);

		last_update_time = curr;
	}
	
	// this->dusk++;
//
// 	//use dusk as a counter to see how fast this loop is going
// 	if (curr - last_count_time > 1000) {
// 		last_count_time = curr;
// 		this->msngr->lowcar_printf("loops in past second: %d", this->dusk);
// 		this->dusk = 0;
// 	}

}
