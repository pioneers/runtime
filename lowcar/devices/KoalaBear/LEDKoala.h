#ifndef LEDKOALA_H
#define LEDKOALA_H

#include "pindefs_koala.h"

// class for controlling the LED's on the KoalaBear
class LEDKoala {
  public:
    // Sets LEDs to output and disabled
    LEDKoala();

    /**
     * Sets the state of the LEDs based on velocity.
     * Arguments:
     *    vel: The velocity of the motors
     *    deadband: The threshold to treat a small velocity as "stop"
     *    enabled: Whether or not to enable the LED
     *      if set to false, the LEDs will be turned off.
     */
    void ctrl_LEDs(float vel, float deadband, bool enabled);

    /**
     * Blinks all LEDs once at the same time to verify that they work
     */
    void test_LEDs();

    /**
     * Sets LED pins to output
     */
    void setup_LEDs();

  private:
    bool red_state;
    bool yellow_state;
    bool green_state;

    // decides when the red LED is on
    void ctrl_red(float vel, float deadband, bool enabled);

    // decides when the yellow LED is on
    void ctrl_yellow(float vel, float deadband, bool enabled);

    // decides when the green LED is on
    void ctrl_green(float vel, float deadband, bool enabled);
};

#endif /* LEDKOALA_H */
