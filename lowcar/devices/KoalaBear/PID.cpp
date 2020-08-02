#include "PID.h"

PID::PID() {
    this->kp = this->ki = this->kd = 0.0;
    this->prev_error = this->prev_pos = this->prev_desired_pos = 0.0;
    this->integral = 0.0;
    this->target_speed = 0.0;
    this->prev_time = micros();
}

/* 
 * Computes a value between -1 and 1 inclusive to tell how to
 * adjust the motor controller pins.
 * Arguments:
 *    curr_pos: current value of the encoder
 */
float PID::compute(float curr_pos) {
    unsigned long curr_time = micros(); // get the current time
    float interval_secs = ((float)(curr_time - this->prev_time)) / 1000000.0; // compute time passed between loops, in seconds
    float desired_pos = prev_desired_pos + (duty_cycle_to_tps(this->target_speed) * interval_secs); // compute the desired position at this time
    float error = desired_pos - curr_pos; // compute the error as the set point (desired position) - process variable (current position)
    this->integral += error * (interval_secs); //compute the new value of this->integral using right-rectangle approximation
    
    // store the current values into previous values for use on next loop
    this->prev_error = error;
    this->prev_pos = curr_pos;
    this->prev_desired_pos = desired_pos;
    this->prev_time = curr_time;
    
    // return kp * error + ki * integral of error * kd * "derivative" of error
    return (this->kp * error) + (this->ki * this->integral) + (this->kd * (error - this->prev_error));
}

void PID::set_coefficients(float kp, float ki, float kd) {
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
}

void PID::set_target_speed(float target_speed) {
    this->target_speed = target_speed;
}

// used when student sets encoder to a certain value
void PID::set_pos_to(float curr_pos) {
    this->prev_pos = curr_pos;
    this->prev_error = 0.0; // resets the error too
}

// three getter functions for the three coefficients
float PID::get_kp() { return this->kp; }
float PID::get_ki() { return this->ki; }
float PID::get_kd() { return this->kd; }

// *********************** HELPER FUNCTIONS *********************** //

// converts speed in units of duty cycle (-1.0 to 1.0)
// to units of encoder ticks per second, tps
float PID::duty_cycle_to_tps(float duty_cycle) {
    return 300.0 * duty_cycle; // TODO: determine this function. will probably be of the form y = kx (if not can go do a regression on a calculator after some tests)
}
