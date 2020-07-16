#include <DummyDevice.h>
#include <defs.h>
#include <Device.h>
#include <Messenger.h>
#include <StatusLED.h>

Device *device; //declare the device

void setup() {
  //device = new Device(DeviceType::LINE_FOLLOWER, 13, 2000, 1000);
  device = new DummyDevice();
}

void loop() {
  device->loop();
} 
