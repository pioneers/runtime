/**
 * A class to handle whether to turn on or off the LED on the Arduino.
 */

#ifndef STATUSLED_H
#define STATUSLED_H

#include "defs.h"

class StatusLED {
  public:
    // sets up the status LED and configures the pin
    StatusLED();

    // toggles LED between on and off
    void toggle();

    // Blinks quickly NUM times
    void quick_blink(int num);

    // Blink slowly NUM times
    void slow_blink(int num);

  private:
    const static int LED_PIN = LED_BUILTIN;  // pin to control the LED
    bool led_enabled;                        // keeps track of whether LED is on or off

    // Blinks NUM times, waiting MS ms between the toggles, and SPACE ms between each blink
    void blink(int num, int ms, int space);
};

#endif
