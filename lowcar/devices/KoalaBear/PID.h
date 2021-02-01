#ifndef PID_h
#define PID_h

#include "Arduino.h"

// Handles the PID controller on one motor
class PID {
  public:
    PID();

    /**
     * Computes a value between -1 and 1 inclusive to tell how to
     * adjust the motor controller pins.
     * Arguments:
     *    curr_pos: current value of the encoder, as a float
     */
    float compute(float curr_pos);

    // Private field setters
    void set_coefficients(float kp, float ki, float kd);
    void set_velocity(float velocity);
    void set_position(float curr_pos);

    // Private field getters
    float get_kp();
    float get_ki();
    float get_kd();

  private:
    float kp, ki, kd;
    float prev_pos, prev_desired_pos;
    float velocity;
    float integral;
    float *prev_error, *prev_time;

    // ************************** HELPER FUNCTIONS ************************** //
    
    /**
     * Finds the sum of the first i elements of the provided array
     * Arguments:
     *    nums: Array of elements to be summed
     *    i: Number of elements of nums to sum
     * Returns:
     *    nums[0] + nums[1] + ... + nums[i - 1]
     */
    float sum(float nums[], int i);
    
    /**
     * Finds the average of the first i elements of the provided array
     * Arguments:
     *    nums: Array of elements to be averaged
     *    i: Number of elements of nums to use in computing average
     * Returns:
     *    (nums[0] + nums[1] + ... + nums[i - 1]) / i
     */
    float average(float nums[], int i);
    
    /**
     * Computes the linear least squares regression of the points
     * given by x_vals and y_vals, where the x_vals array contains
     * the x values of the points to use and y_vals array contains
     * the y values of the points to use. The number of points to use
     * is given by the number argument.
     * Arguments:
     *    x_vals: Array of x values of the points to calculate regression through
     *    y_vals: Array of y values of the points to calculate regression through
     *    number: Number of points to use in calculating regression
     * Returns:
     *    Slope of regression line through the given points
     */
    float regression (float x_vals[], float y_vals[], int number);

    /**
     * Converts speed (in duty cycle units from -1.0 to 1.0) to
     * encoder ticks per second (tps)
     * Arguments:
     *    duty_cycle: The duty cycle to convert to encoder ticks
     * Returns:
     *    the converted value to encoder ticks
     */
    float velocity_to_tps(float velocity);
};

#endif
