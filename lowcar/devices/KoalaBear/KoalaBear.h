#ifndef KOALABEAR_H
#define KOALABEAR_H

#include <SPI.h>

#include "Device.h"
#include "LEDKoala.h"
#include "PID.h"
#include "defs.h"
#include "pindefs_koala.h"

class KoalaBear : public Device {
  public:
    /**
     * Constructor. Initializes motors, encoders, and PID controllers
     */
    KoalaBear();

    virtual size_t device_read(uint8_t param, uint8_t* data_buf);
    virtual size_t device_write(uint8_t param, uint8_t* data_buf);

    /**
     * Does electrical setup, setup pins, encoders, and LEDs
     */
    virtual void device_enable();

    /**
     * Sets all PID coefficients to default values, stops motors, and enables PID.
     */
    virtual void device_reset();

    /**
     * Sets LEDs and moves motors according to current target speeds.
     */
    virtual void device_actions();

  private:
    float velocity_a, velocity_b;           // student-specified velocities of motors, range [-1.0, 1.0]
    float deadband_a, deadband_b;           // deadbands of motors
    uint8_t invert_a, invert_b;             // true if Motor A should rotate in opposite direction of default; false for default direction
    uint8_t pid_enabled_a, pid_enabled_b;   // true if using PID control; false if using manual drive mode
    float curr_velocity_a, curr_velocity_b; // current velocity of motors
    unsigned long prev_loop_time;           // for calculating acceleration
    uint8_t online;                         // true if controller is online and in nominal loop; false otherwise
    PID *pid_a, *pid_b;                     // PID controllers for motors
    LEDKoala* led;                          // for controlling the KoalaBear LED
    unsigned long prev_led_time;
    int curr_led_mtr;

    /**
     * Moves a motor given a target "velocity"
     * Negative indicates moving backwards; Positive indicates moving forwards.
     * Arguments:
     *    target: value between -1 and 1 inclusive indicating how much to move the motor
     *    mtr: The motor to move
     */
    void drive(float target, uint8_t mtr);

    // Writes to pins to setup hardware.
    // Ask team electrical for how it works and what it does
    void electrical_setup();

    // Helper functions for electrical_setup()
    void write_current_lim();
    void read_current_lim();
};

#endif
