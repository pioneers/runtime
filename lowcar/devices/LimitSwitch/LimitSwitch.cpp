#include "LimitSwitch.h"

// number of switches on a limit switch (how many input pins)
const int LimitSwitch::NUM_SWITCHES = 3;
const uint8_t LimitSwitch::pins[] = {
    (const uint8_t) Analog::IO0,
    (const uint8_t) Analog::IO1,
    (const uint8_t) Analog::IO2};

// default constructor simply specifies DeviceID and year to generic constructor
LimitSwitch::LimitSwitch() : Device(DeviceType::LIMIT_SWITCH, 0) {
    ;
}

size_t LimitSwitch::device_read(uint8_t param, uint8_t* data_buf) {
    // put pin value into data_buf and return the state of the switch
    if (param < NUM_SWITCHES && param >= 0) {
        data_buf[0] = (digitalRead(this->pins[param]) == LOW);
        return sizeof(uint8_t);
    } else {
        return 0;
    }
}

void LimitSwitch::device_enable() {
    // set all pins to INPUT mode
    for (int i = 0; i < LimitSwitch::NUM_SWITCHES; i++) {
        pinMode(LimitSwitch::pins[i], INPUT);
    }
}
