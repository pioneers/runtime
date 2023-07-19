#include "ServoControl.h"

const int ServoControl::NUM_SERVOS = 2;
const int ServoControl::SERVO_0 = 5;
const int ServoControl::SERVO_1 = 6;
const int ServoControl::SERVO_CENTER = 1500;  // center position on servo, in microseconds (?)
const int ServoControl::SERVO_RANGE = 1000;   // range of movement of servo is SERVO_CENTER +/- SERVO_RANGE
const uint8_t ServoControl::pins[] = {SERVO_0, SERVO_1};

Servo* servo0 = new Servo();
Servo* servo1 = new Servo();
Servo servos[2] = {*servo0, *servo1};

ServoControl::ServoControl() : Device(DeviceType::SERVO_CONTROL, 1) {
    this->positions = new float[NUM_SERVOS];
    for (int i = 0; i < ServoControl::NUM_SERVOS; i++) {
        this->positions[i] = 0.0;
    }
    disable_all();
}

size_t ServoControl::device_read(uint8_t param, uint8_t* data_buf) {
    if (param < NUM_SERVOS && param >= 0) {
        float* float_buf = (float*) data_buf;
        float_buf[0] = this->positions[param];
        return sizeof(float);
    } else {
        return 0;
    }

}

size_t ServoControl::device_write(uint8_t param, uint8_t* data_buf) {
    if (param < NUM_SERVOS && param >= 0) {
        float value = ((float*) data_buf)[0];
        if (value < -1 || value > 1) {
            return 0;
        }

        // if servo isn't attached yet, attach the pin to the servo object
        if (!servos[param].attached()) {
            servos[param].attach(this->pins[param]);
        }
        this->positions[param] = value;
        servos[param].writeMicroseconds(ServoControl::SERVO_CENTER + (this->positions[param] * ServoControl::SERVO_RANGE));
        return sizeof(float);
    } else {
        return 0;
    }
}

void ServoControl::device_reset() {
    disable_all();
}

void ServoControl::disable_all() {
    for (int i = 0; i < ServoControl::NUM_SERVOS; i++) {
        servos[i].detach();
    }
}
