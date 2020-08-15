#ifndef PID_h
#define PID_h

#include "Arduino.h"

// Handles the PID controller on one motor
class PID {
public:
	PID();
	float compute(float curr_pos);
        
	void set_coefficients(float kp, float ki, float kd);
	void set_target_speed(float new_target_speed);
    void set_position(float curr_pos);
    
	float get_kp();
	float get_ki();
	float get_kd();

private:
	float kp, ki, kd;
    float prev_error, prev_pos, prev_desired_pos;
    float target_speed;
    float integral;
    unsigned long prev_time;
    
    // *********************** HELPER FUNCTIONS *********************** //
    
    float duty_cycle_to_tps(float duty_cycle);
};

#endif
