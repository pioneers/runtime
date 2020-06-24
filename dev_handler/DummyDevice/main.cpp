/*
 * The main script to be flashed onto the Arduino
 */

#include "Device.h"

Device *device; //declare the device

void setup() {
	device = new Device(DeviceID::LIMIT_SWITCH, 0, 2000, 2000);
}

void loop() {
	device->loop();
}
