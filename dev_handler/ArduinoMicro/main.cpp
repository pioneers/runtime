/*
 * The main script to be flashed onto the Arduino
 */

#include "Device.h"

Device *device; //declare the device

void setup() {
	device = new Device();
}

void loop() {
	device->loop();
}
