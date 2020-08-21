#ifndef KOALABEAR_H
#define KOALABEAR_H

#include <SPI.h>

#include "Device.h"
#include "PID.h"
#include "LEDKoala.h"
#include "pindefs_koala.h"
#include "defs.h"

class KoalaBear : public Device {
public:
    /**
     * Constructor. Initializes motors, encoders, and PID controllers
     */
    KoalaBear ();

    virtual size_t device_read(uint8_t param, uint8_t *data_buf);
    virtual size_t device_write(uint8_t param, uint8_t *data_buf);

    /**
     * Does electrical setup, setup pins, encoders, and LEDs
     */
    virtual void device_enable();

    /**
     * Sets all PID coefficients to 0, stops motors, and disables PID.
     */
    virtual void device_disable();

    /**
     * Sets LEDs and moves motors according to current target speeds.
     */
    virtual void device_actions();

private:
    float target_speed_a, target_speed_b; // target speeds of motors
    float deadband_a, deadband_b;         // deadbands of motors
    uint8_t pid_enabled_a, pid_enabled_b; // whether or not motors are enabled
    volatile uint32_t enc_a, enc_b;       // encoder values of motors
    PID *pid_a, *pid_b;                   // PID controllers for motors
    LEDKoala *led;                        // for controlling the KoalaBear LED
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
