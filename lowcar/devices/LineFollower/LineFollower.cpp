#include "LineFollower.h"

// pins that the line follower reads data from (defined in defs.h)
const uint8_t LineFollower::pins[] = {
    (const uint8_t) Analog::IO0,
    (const uint8_t) Analog::IO1,
    (const uint8_t) Analog::IO2};
const int LineFollower::NUM_PINS = 3;  // number of pins used for I/O for LineFollower

LineFollower::LineFollower() : Device(DeviceType::LINE_FOLLOWER, 1) {
    ;
}

size_t LineFollower::device_read(uint8_t param, uint8_t* data_buf) {
    // use data_ptr_float to shove 10-bit sensor reading into the data_buf
    if (param < NUM_PINS && param >= 0) {
        float* data_ptr_float = (float*) data_buf;
        *data_ptr_float = 1.0 - ((float) analogRead(this->pins[param])) / 1023.0;
        return sizeof(float);
    } else {
        return 0;
    }
}

void LineFollower::device_enable() {
    // set all pins to INPUT mode
    for (int i = 0; i < LineFollower::NUM_PINS; i++) {
        pinMode(LineFollower::pins[i], INPUT);
    }
}
