#include "PID.h"

#define MAX_TPS 2700.0   // maximum encoder ticks per second that a motor can go (determined empirically)

PID::PID() {
    this->kp = this->ki = this->kd = 0.0;
    this->prev_error = this->prev_pos = this->prev_desired_pos = 0.0;
    this->integral = 0.0;
    this->velocity = 0.0;
    this->prev_time = micros();
}

float PID::compute(float curr_pos) {
    unsigned long curr_time = micros();                                                                 // get the current time
    float interval_secs = ((float) (curr_time - this->prev_time)) / 1000000.0;                         	// compute time passed between loops, in seconds
    float desired_pos = this->prev_desired_pos + (velocity_to_tps(this->velocity) * interval_secs);		// compute the desired position at this time
    float error = desired_pos - curr_pos;                                                               // compute the error as the set point (desired position) - process variable (current position)
    this->integral += error * interval_secs;                                                            // compute the new value of this->integral using right-rectangle approximation

    // output = kp * error + ki * integral of error * kd * "derivative" of error
    float output = (this->kp * error) + (this->ki * this->integral) + (this->kd * ((error - this->prev_error) / interval_secs));

    // store the current values into previous values for use on next loop
    this->prev_error = error;
    this->prev_pos = curr_pos;
    this->prev_desired_pos = desired_pos;
    this->prev_time = curr_time;
	
	// if target speed is 0, output 0 automatically (prevent jittering when motor stopped) and reset integral to 0
	if (this->velocity == 0.0) {
		this->integral = 0.0;
		return 0.0;
	} else {
		return output;
	}
}

void PID::set_coefficients(float kp, float ki, float kd) {
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
}

void PID::set_velocity(float velocity) {
    this->velocity = velocity;
}

// used when student sets encoder to a certain value
void PID::set_position(float curr_pos) {
    this->prev_pos = curr_pos;
    this->prev_error = 0.0;  // resets the error too
}

// three getter functions for the three coefficients
float PID::get_kp() { return this->kp; }
float PID::get_ki() { return this->ki; }
float PID::get_kd() { return this->kd; }

// *********************** HELPER FUNCTIONS *********************** //

float PID::velocity_to_tps(float velocity) {
    return MAX_TPS * velocity;
}
