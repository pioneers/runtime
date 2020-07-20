#include <DummyDevice.h>

// flash script will append a #include above this line to include the necessary library for the requested device
#include <defs.h>
#include <Device.h>
#include <Messenger.h>
#include <StatusLED.h>

// THIS FILE IS USED BY THE FLASH SCRIPT TO BUILD THE ACTUAL DummyDevice.INO FILE (which will be located in Device/Device.ino)

Device *device; //declare the device

void setup() {
	device = new DummyDevice(); //flash script will replace DEVICE with requested Device type
	device->set_uid(3751473823233038656); //flash script will replace UID_INSERTED with randomly generated 64-bit value
}

void loop() {
	device->loop();
} 
