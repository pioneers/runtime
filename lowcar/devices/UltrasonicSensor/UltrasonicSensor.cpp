#include "UltrasonicSensor.h"

// pin definitions for the ultrasonic sensor
#define TRIG_PIN 16
#define ECHO_PIN 14

UltrasonicSensor::UltrasonicSensor() : Device(DeviceType::ULTRASONIC_SENSOR, 1) {
    ;
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

    float* float_buf = (float*) data_buf;
    *float_buf = distance;

    return sizeof(float);
}

void UltrasonicSensor::device_enable() {
    // set all pins to INPUT mode
    this->trigPin = TRIG_PIN;
    this->echoPin = ECHO_PIN;

    pinMode(this->trigPin, OUTPUT);
    pinMode(this->echoPin, INPUT);
}