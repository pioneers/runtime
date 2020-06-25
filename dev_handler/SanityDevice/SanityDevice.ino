/*
 * A very simple device to test that libusb works
 */
#include <Arduino.h>

#define LED_PIN 13

/* Set the LED to output */
void setup() {
    pinMode(LED_PIN, OUTPUT);
}

/* Every other second, toggle the LED */
void loop() {
    digitalWrite(LED_PIN, LOW);
    delay(1000);
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
}
