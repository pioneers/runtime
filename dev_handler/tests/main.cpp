/**
 *  The main file for the fake device to test the dev handler
 */
#include "Device.h"
#include "defs.h"

Device *device;

void setup() {
    device = new Device(DeviceID::LIMIT_SWITCH, 0, 2000, 1000);
}

void loop() {
    device->loop();
}

/* Arduino execution */
int main() {
    setup();
    while (1) {
        loop();
    }
}
