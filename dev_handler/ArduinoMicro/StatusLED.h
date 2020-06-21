/**
 * A class to handle whether to turn on or off the LED on the Arduino.
 * The LED's state is toggled when there are parsing errors (See Device.cpp)
 */

#ifndef STATUSLED_H
#define STATUSLED_H

#include "defs.h"

class StatusLED
{
public:
	//sets up the status LED and configures the pin
	StatusLED ();

	//toggles LED between on and off
	void toggle ();

private:
	const static int LED_PIN = 13; //pin to control the LED
	bool led_enabled; //keeps track of whether LED is on or off
};

#endif
