#include "StatusLED.h"

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
	this->blink(num, 200, 200);
}

void StatusLED::slow_blink (int num)
{
	this->blink(num, 600, 200);
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
