#include "PID.h"

#define MAX_TPS 2700.0  // maximum encoder ticks per second that a motor can go (determined empirically)
#define SAMPLES 5		// number of samples for calculating the derivative

PID::PID() {
    this->kp = this->ki = this->kd = 0.0;
    this->prev_pos = this->prev_desired_pos = 0.0;
    this->integral = 0.0;
    this->velocity = 0.0;
	
	this->prev_error = new float[SAMPLES];
    this->prev_time = new unsigned long[SAMPLES];
	for (int i = 0; i < SAMPLES; i++) {
		this->prev_error[i] = 0.0;
		this->prev_time[i] = micros();
	}
}

float PID::compute(float curr_pos) {
    unsigned long curr_time = micros();                                                              // get the current time
    float interval_secs = ((float) (curr_time - this->prev_time[SAMPLES - 1])) / 1000000.0;          // compute time passed between loops, in seconds
    float desired_pos = this->prev_desired_pos + (velocity_to_tps(this->velocity) * interval_secs);  // compute the desired position at this time
    float error = desired_pos - curr_pos;                                                            // compute the error as the set point (desired position) - process variable (current position)
    this->integral += error * interval_secs;                                                         // compute the new value of this->integral using right-rectangle approximation
	
	// shift the prev_error and prev_time arrays and insert curr_time and error
	for (int i = 0; i < SAMPLES - 1; i++) {
		this->prev_error[i] = this->prev_error[i + 1];
		this->prev_time[i] = this->prev_time[i + 1];
	}
	this->prev_error[SAMPLES - 1] = error;
	this->prev_time[SAMPLES - 1] = curr_time;

    // output = kp * error + ki * integral of error * kd * "derivative" of error
    float output = (this->kp * error) + (this->ki * this->integral) + (this->kd * regression(this->prev_time, this->prev_error, SAMPLES));

    // store the current values into previous values for use on next loop
    this->prev_pos = curr_pos;
    this->prev_desired_pos = desired_pos;

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
	for (int i = 0; i < SAMPLES - 1; i++) {
		this->prev_error[i] = 0.0; // resets the error too
	}
}

// three getter functions for the three coefficients
float PID::get_kp() { return this->kp; }
float PID::get_ki() { return this->ki; }
float PID::get_kd() { return this->kd; }

// *********************** HELPER FUNCTIONS *********************** //

// find the sum of the given floats
float PID::sum(float nums[], int i) {
	float total = 0.0;
	for (int x = 0; x < i; x++) {
		total += nums[x];
	}
	return total;
}

// find the average of the given floats
float PID::average(float nums[], int i) {
	float total = sum(nums, i);
	return (total / (float)i);
}

// calculate the slope of least-squares linear regression through points
// "number" is the number of elements in the arrays
float PID::regression(unsigned long x_vals[], float y_vals[], int number) {
    float x_avg, y_avg, slope;
    float numerator[number], denominator[number], x_vals_float[number];
	
	// convert inputted x_vals to floats
	for (int i = 0; i < number; i++) {
		x_vals_float[i] = (float) x_vals[i];
	}
	
    //calculate averages
    x_avg = average(x_vals_float, number);
    y_avg = average(y_vals, number); 

    for (int i = 0; i < number; i++) {
        numerator[i] = ((x_vals_float[i] - x_avg) * (y_vals[i] - y_avg));			// (x_i - x_avg) * (y_i - y_avg)
        denominator[i] = ((x_vals_float[i] - x_avg) * (x_vals_float[i] - x_avg));	// (x_i - x_avg) ^ 2
    }
    slope = (sum(numerator, number) / sum(denominator, number)); 

    return slope;
}

float PID::velocity_to_tps(float velocity) {
    return MAX_TPS * velocity;
}
