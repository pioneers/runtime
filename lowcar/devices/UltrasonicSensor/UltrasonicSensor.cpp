#include "UltrasonicSensor.h"

// pin definitions for the ultrasonic sensor
#define TRIG_PIN 14
#define ECHO_PIN 16

UltrasonicSensor::UltrasonicSensor() : Device(DeviceType::ULTRASONIC_SENSOR, 17) {
    this->distance = 0;
}

size_t UltrasonicSensor::device_read(uint8_t param, uint8_t* data_buf) {
    float* float_buf = (float*) data_buf;
    float_buf[0] = this->distance;

    return sizeof(float);
}

void UltrasonicSensor::device_enable() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
}

void UltrasonicSensor::device_actions() {
    // trigger the ultrasonic pulse
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(10);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * 0.034f / 2.0f;  // convert to cm

    this->distance = distance;
}