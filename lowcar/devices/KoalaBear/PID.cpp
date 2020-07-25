#include "PID.h"

#define encoder0PinA	14
#define encoder0PinB	15

PID::PID(float SetPoint, float KP, float KI, float KD, float initTime) {
	this->kp = KP;
	this->ki = KI;
	this->kd = KD;
	this->setPoint = SetPoint;
	this->lastTime = initTime;

	this->enc = new Encoder(encoder0PinA, encoder0PinB);
	this->enc->write(0); //reset the encoder because why not
	this->prevPos = enc->read();
	this->prevVel = 0.0;
}

/* Computes a value between -1 and 1 inclusive to tell how to
 * adjust the motor controller pins.
 * This is called continually by PolarBear.
 * Also updates the encoder position and velocity with each call. */
float PID::compute() {
	//updateVel();
	float currTime = (float)(millis());
	float deltaTime = currTime - this->lastTime;


	//float goalPos = prevPos + (prevVel + setPoint)*(deltaTime) / 2
	//float error = goalPos - enc->read();	//uses encoder reading as current position
	float error = this->setPoint - this->enc->read();	//uses encoder reading as current position


	this->errorSum += error * deltaTime;
	float deriv = (error - this->lastError) / deltaTime;
	this->lastTime = currTime;
	float out = (this->kp * error) + (this->ki * this->errorSum) + (this->kd * deriv);


	this->prevPos = this->enc->read();
	//prevVel = setPoint;
	if (out > 1)
	{
		return 1;
	}
	else if (out < -1)
	{
		return -1;
	}
	return out;
}

void PID::setCoefficients(float KP, float KI, float KD) {
	this->kp = KP;
	this->ki = KI;
	this->kd = KD;
}

void PID::setSetpoint(float sp) { this->setPoint = sp; }

float PID::getKP() { return this->kp; }
float PID::getKI() { return this->ki; }
float PID::getKD() { return this->kd; }

float PID::readPos() { return this->enc->read(); }
float PID::readVel() { return 0.00; }

/****************************************************
* 				ENCODER FUNCTIONS
****************************************************/
long res = 100;
long interval_us = res*(2500/11); //interval in us; 2500/11 == 1000000 (1 sec) / 4400 (tick range)

void PID::encoderSetup() {
  pinMode(encoder0PinA,INPUT);
  pinMode(encoder0PinB,INPUT);
}

void PID::resetEncoder() {
  enc->write(0);
}

// Updates the encoder's velocity by calculating the position slope.
void PID::updateVel() {
  float enc_reading = this->enc->read();
  this->prevVel = ((enc_reading - this->prevPos)*1000000)/((float) interval_us);
  this->prevPos = enc_reading; // Save the current pos
}

/**************
Original Encoder
**************/
/*
long res = 100;
long interval_us = res*(2500/11); //interval in us; 2500/11 == 1000000 (1 sec) / 4400 (tick range)
float pos = 0;
float vel = 0;
volatile signed long old_encoder0Pos = 0;

void PID::updateEncoder() {
  updatePos();
  updateVel();
}

void PID::encoderSetup() {
  pinMode(encoder0PinA,INPUT);
  pinMode(encoder0PinB,INPUT);
}

void PID::resetEncoder() {
  enc->write(0);
}

float PID::readPos() {
  return pos;
}

float PID::readVel() {
  return vel;
}

void PID::updatePos() {
  pos = enc->read();
}
*/
