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
     *    curr_pos: current value of the encoder, as a double
     */
    double compute(double curr_pos);

    // Private field setters
    void set_coefficients(double kp, double ki, double kd);
    void set_velocity(double velocity);
    void set_position(double curr_pos);

    // Private field getters
    double get_kp();
    double get_ki();
    double get_kd();

  private:
    double kp, ki, kd;
    double prev_pos, prev_desired_pos;
    double velocity;
    double integral;
    double *prev_error, *prev_time;

    // ************************** HELPER FUNCTIONS ************************** //

    /**
     * Finds the sum of the first i elements of the provided array
     * Arguments:
     *    nums: Array of elements to be summed
     *    i: Number of elements of nums to sum
     * Returns:
     *    nums[0] + nums[1] + ... + nums[i - 1]
     */
    double sum(double nums[], int i);

    /**
     * Finds the average of the first i elements of the provided array
     * Arguments:
     *    nums: Array of elements to be averaged
     *    i: Number of elements of nums to use in computing average
     * Returns:
     *    (nums[0] + nums[1] + ... + nums[i - 1]) / i
     */
    double average(double nums[], int i);

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
    double regression(double x_vals[], double y_vals[], int number);

    /**
     * Converts speed (in duty cycle units from -1.0 to 1.0) to
     * encoder ticks per second (tps)
     * Arguments:
     *    duty_cycle: The duty cycle to convert to encoder ticks
     * Returns:
     *    the converted value to encoder ticks
     */
    double velocity_to_tps(double velocity);
};

#endif
