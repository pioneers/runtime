#ifndef POLARBEAR_H
#define POLARBEAR_H

#include "Device.h"
#include "PID.h"
#include "LED.h"
#include "defs.h"

#define FEEDBACK A3   // pin for getting motor current
#define PWM1 6        // pwm pin 1
#define PWM2 9        // pwm pin 2

class PolarBear : public Device {
public:
	PolarBear();
	virtual size_t device_read(uint8_t param, uint8_t *data_buf);
	virtual size_t device_write(uint8_t param, uint8_t *data_buf);
	virtual void device_enable();
	virtual void device_disable();
	virtual void device_actions();

private:
	float duty_cycle;
	float deadband;
	float dpwm_dt;
	void drive(float target);
};

#endif
