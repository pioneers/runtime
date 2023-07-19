#include "PID.h"

#define MAX_TPS 2700.0  // maximum encoder ticks per second that a motor can go (determined empirically)
#define SAMPLES 5       // number of samples for calculating the derivative
#define ONE_MILLION 1000000.0

PID::PID() {
    this->kp = this->ki = this->kd = 0.0;
    this->prev_pos = this->prev_desired_pos = 0.0;
    this->integral = 0.0;
    this->velocity = 0.0;

    // create and populate the prev_error and prev_time arrays
    this->prev_error = new double[SAMPLES];
    this->prev_time = new double[SAMPLES];
    for (int i = 0; i < SAMPLES; i++) {
        this->prev_error[i] = 0.0;
        this->prev_time[i] = ((double) micros()) / ONE_MILLION;
    }
}

double PID::compute(double curr_pos) {
    double curr_time = ((double) micros()) / ONE_MILLION;                                              // get the current time, in seconds
    double interval_secs = curr_time - this->prev_time[SAMPLES - 1];                                  // compute time passed between loops, in seconds
    double desired_pos = this->prev_desired_pos + (velocity_to_tps(this->velocity) * interval_secs);  // compute the desired position at this time
    double error = desired_pos - curr_pos;                                                            // compute the error as the set point (desired position) - process variable (current position)
    this->integral += error * interval_secs;                                                         // compute the new value of this->integral using right-rectangle approximation

    // shift the prev_error and prev_time arrays and insert curr_time and error
    for (int i = 0; i < SAMPLES - 1; i++) {
        this->prev_error[i] = this->prev_error[i + 1];
        this->prev_time[i] = this->prev_time[i + 1];
    }
    this->prev_error[SAMPLES - 1] = error;
    this->prev_time[SAMPLES - 1] = curr_time;

    // output = kp * error + ki * integral of error * kd * "derivative" of error
    double output = (this->kp * error) + (this->ki * this->integral) + (this->kd * regression(this->prev_time, this->prev_error, SAMPLES));

    // store the current values into previous values for use on next loop
    this->prev_pos = curr_pos;
    this->prev_desired_pos = desired_pos;

    // if target speed is 0, output 0 automatically (prevent jittering when motor stopped) and reset integral to 0
    if (this->velocity == 0.0) {
        this->integral = 0.0;
        this->prev_desired_pos = curr_pos;  // this will flush out the values in the error array
        return 0.0;
    } else {
        return output;
    }
}

void PID::set_coefficients(double kp, double ki, double kd) {
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
}

void PID::set_velocity(double velocity) {
    this->velocity = velocity;
}

// used when student sets encoder to a certain value
// resets all of the PID controller values
void PID::set_position(double curr_pos) {
    this->prev_pos = this->prev_desired_pos = curr_pos;
    for (int i = 0; i < SAMPLES - 1; i++) {
        this->prev_error[i] = 0.0;
        this->prev_time[i] = ((double) micros()) / ONE_MILLION;
        this->integral = 0.0;
    }
}

// three getter functions for the three coefficients
double PID::get_kp() { return this->kp; }
double PID::get_ki() { return this->ki; }
double PID::get_kd() { return this->kd; }

// *********************** HELPER FUNCTIONS *********************** //

// find the sum of the given doubles
double PID::sum(double nums[], int i) {
    double total = 0.0;
    for (int x = 0; x < i; x++) {
        total += nums[x];
    }
    return total;
}

// find the average of the given doubles
double PID::average(double nums[], int i) {
    double total = sum(nums, i);
    return (total / (double) i);
}

// calculate the slope of least-squares linear regression through points
// "number" is the number of elements in the arrays
double PID::regression(double x_vals[], double y_vals[], int number) {
    double x_avg, y_avg, slope;
    double numerator[number], denominator[number];

    // calculate averages
    x_avg = average(x_vals, number);
    y_avg = average(y_vals, number);

    for (int i = 0; i < number; i++) {
        numerator[i] = ((x_vals[i] - x_avg) * (y_vals[i] - y_avg));    // (x_i - x_avg) * (y_i - y_avg)
        denominator[i] = ((x_vals[i] - x_avg) * (x_vals[i] - x_avg));  // (x_i - x_avg) ^ 2
    }
    slope = (sum(numerator, number) / sum(denominator, number));

    return slope;
}

double PID::velocity_to_tps(double velocity) {
    return MAX_TPS * velocity;
}
