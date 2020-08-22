#ifndef POLARBEAR_H
#define POLARBEAR_H

#include "Device.h"
#include "LED.h"
#include "PID.h"
#include "defs.h"

#define FEEDBACK A3  // pin for getting motor current
#define PWM1 6       // pwm pin 1
#define PWM2 9       // pwm pin 2

class PolarBear : public Device {
  public:
    PolarBear();
    virtual size_t device_read(uint8_t param, uint8_t* data_buf);
    virtual size_t device_write(uint8_t param, uint8_t* data_buf);

    /**
     * Setup LEDs, and set FEEDBACk to input and PWM1 & PWM2 to output
     */
    virtual void device_enable();

    /**
     * Stops the motors
     */
    virtual void device_disable();

    /**
     * Updates LEDs and moves the motor according to this->DUTY_CYCLE
     */
    virtual void device_actions();

  private:
    // The "velocity" to move the motor
    // (range -1, 1 inclusive, where 1 is max speed forward, -1 is max speed backwards)
    // 0 is stop
    float duty_cycle;
    // The minimum threshold duty_cycle for it to be actually written to in device_write()
    float deadband;
    float dpwm_dt;

    /**
     * Moves the motor given a target "velocity"
     * Negative indicates moving backwards; Positive indicates moving forwards.
     * Arguments:
     *    target: value between -1 and 1 inclusive indicating how much to move the motor
     */
    void drive(float target);
};

#endif
