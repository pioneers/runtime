
#include "Device.h"
#include "defs.h"
Device *device; //declare the device

void setup() {
  device = new Device(DeviceType::LINE_FOLLOWER, 13, 2000, 1000);
}

void loop() {
  device->loop();
}
