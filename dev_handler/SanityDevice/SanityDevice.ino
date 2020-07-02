/*
 * A very simple device to test that libusb works
 */
#include <Arduino.h>

#define RXLED 17 // One LED is on pin 17

#define QUICK_TIME 300 // Feel free to change this
// Morse code ratios
#define SLOW_TIME (QUICK_TIME*3)
#define PAUSE_TIME QUICK_TIME

void setup() {
    pinMode(RXLED, OUTPUT);
	// pinMode(TXLED, OUTPUT); This is unnecessary for this special LED
	RXLED0;
	TXLED0;
}

/* Every three seconds, try to read something */
void loop() {
    //delay(2000); // Wait a bit for dev handler to send Ping
    read_message();
    //delay(100000);
}

void read_message ()
{
	// Return if there's nothing to read
	if (!Serial.available()) {
        return;
	}

	// Find the start of the packet
	int last_byte_read = -1;
	while (Serial.available()) {
		last_byte_read = Serial.read();
		if (last_byte_read == 0) {
      	break;
		}
	}

    // If couldn't find where the message started, return
	if (last_byte_read != 0) {
        slow_blink(3);
		return;
	}

    // Broken message (rest of the message is gone)
	if (Serial.available() == 0 || Serial.peek() == 0) {
        slow_blink(10);
		return;
	}

    // We got a message
    quick_blink(10);
    //delay(2000);

    // Read the rest of the message
    int cobs_len = Serial.read();
    int total_len = 2 + cobs_len; // delimiter + byte for cobs len + sizeof(message)
   // slow_blink(total_len);
    byte data[total_len];
    data[0] = 0x00;
    data[1] = cobs_len;
    // There are cobs_len bytes left to read into the data buffer
    Serial.readBytesUntil(0x00, &data[2], cobs_len);

    // Parrot back to dev handler what we got
    Serial.write(data, total_len);
   // Serial.write((byte)0x00);
   // Serial.write((byte)0x02);
    quick_blink(1);
    //delay(100000);
}

void quick_blink (int num)
{
	// TXLED0 turns light off
	// TXLED1 turns light on
	for (int i = 0; i < num; i++) {
		TXLED1;
		delay(QUICK_TIME);
		TXLED0;
		delay(PAUSE_TIME);
	}
}

void slow_blink (int num)
{
	// RXLED0 turns light off
	// RXLED1 turns light on
	for (int i = 0; i < num; i++) {
		RXLED1;
		delay(SLOW_TIME);
		RXLED0;
		delay(PAUSE_TIME);
	}
}
