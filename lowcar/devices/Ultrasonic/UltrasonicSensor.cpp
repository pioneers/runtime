#include "UltrasonicSensor.h"
#include <Arduino.h>

// pin definitions for the ultrasonic sensor
const uint8_t UltrasonicSensor::trigPin = (const uint8_t) Digital::IO4;
const uint8_t UltrasonicSensor::echoPin = (const uint8_t) Digital::IO5;

UltrasonicSensor::UltrasonicSensor()
    : Device(DeviceType::DISTANCE_SENSOR, 1) {
    ;
}

void UltrasonicSensor::device_enable() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
}

size_t UltrasonicSensor::device_read(uint8_t param, uint8_t* data_buf) {
    // trigger the ultrasonic pulse
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    float distance = duration * 0.034f / 2.0f;  // convert to cm

    float* data_ptr_float = (float*) data_buf;
    *data_ptr_float = distance;

    return sizeof(float);
}

size_t UltrasonicSensor::device_write(uint8_t param, uint8_t* data_buf) {
    return 0;
}

void UltrasonicSensor::device_actions() {
}