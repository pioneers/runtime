
// flash script will append a #include above this line to include the necessary library for the requested device
#include <Device.h>
#include <Messenger.h>
#include <StatusLED.h>
#include <defs.h>

// THIS FILE IS USED BY THE FLASH SCRIPT TO BUILD THE ACTUAL DEVICE.INO FILE (which will be located in Device/Device.ino)

Device* device;  //declare the device

void setup() {
    device = new DEVICE();          //flash script will replace DEVICE with requested Device type
    device->set_uid(UID_INSERTED);  //flash script will replace UID_INSERTED with randomly generated 64-bit value
}

void loop() {
    device->loop();
}
