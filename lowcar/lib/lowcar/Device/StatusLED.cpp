#include "StatusLED.h"

#define QUICK_TIME 300 // Feel free to change this
// Morse code ratios
#define SLOW_TIME (QUICK_TIME*3)
#define PAUSE_TIME QUICK_TIME

StatusLED::StatusLED ()
{
	pinMode(StatusLED::LED_PIN, OUTPUT);
	digitalWrite(StatusLED::LED_PIN, LOW);
	led_enabled = false;
}

void StatusLED::toggle ()
{
	digitalWrite(StatusLED::LED_PIN, led_enabled ? LOW : HIGH);
	led_enabled = !led_enabled;
}

void StatusLED::quick_blink (int num)
{
	this->blink(num, QUICK_TIME, PAUSE_TIME);
}

void StatusLED::slow_blink (int num)
{
	this->blink(num, SLOW_TIME, PAUSE_TIME);
}

void StatusLED::blink (int num, int ms, int space)
{
	for (int i = 0; i < num; i++) {
		this->toggle();
		delay(ms);
		this->toggle();
		delay(space);
	}
}
