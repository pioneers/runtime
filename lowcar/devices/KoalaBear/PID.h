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
    float prev_error, prev_pos, prev_desired_pos;
    float velocity;
    float integral;
    unsigned long prev_time;

    // ************************** HELPER FUNCTIONS ************************** //

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
