#ifndef PID_h
#define PID_h

#include "Arduino.h"
#include <Encoder.h>

class PID {
public:
	PID(float SetPoint, float KP, float KI, float KD, float initTime);
	float compute();
	void setCoefficients(float KP, float KI, float KD);
	void setSetpoint(float sd);
	float getKP();
	float getKI();
	float getKD();
	/* Below are functions for the encoder. */
	void updateEncoder();
	void encoderSetup();
	void resetEncoder();
	float readPos();
	float readVel();
	void updateVel();
	void updatePos();

private:
	float kp;
	float ki;
	float kd;
	float lastTime;
	float lastError;
	float setPoint;
	float errorSum;
	float prevPos;
	float prevVel;
	Encoder* enc;
};

#endif
