/*
 * A very simple device to test that libusb works
 */
#include <Arduino.h>
#include "StatusLED.h"

StatusLED *led; //declare the device

void setup() {
    pinMode(LED_PIN, OUTPUT);
    led = new StatusLED();
}

/* Every other second, toggle the LED */
void loop() {
    read_message();
}

void read_message ()
{
	//if nothing to read
	if (!Serial.available()) {
        return;
	}

	//find the start of packet
	int last_byte_read = -1;
	while (Serial.available()) {
        led->fast_blink(1); // If found something, blink fast
		last_byte_read = Serial.read();
		if (last_byte_read == 0) {
            led->fast_blink(5); // If found the start of packet, blink fast 5 times
			break;
		}
	}

	if (last_byte_read != 0) { //no start of packet found
        led->slow_blink(3);
		return;
	}
	if (Serial.available() == 0 || Serial.peek() == 0) { //no packet length found
        led->slow_blink(10);
		return;
	}
    // We got a message
    led->fast_blink(20);
}
