#include "UltrasonicSensor.h"

UltrasonicSensor sensor;

void setup() {
  Serial.begin(9600);
  sensor.device_enable();  // sets up TRIG and ECHO
}

void loop() {
  uint8_t buffer[sizeof(float)];
  size_t bytes = sensor.device_read(0, buffer);

  if (bytes == sizeof(float)) {
    float distance;
    memcpy(&distance, buffer, sizeof(float));
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  } else {
    Serial.println("Failed to read sensor data.");
  }

  delay(500);
}
