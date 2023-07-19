#include "LEDKoala.h"
#include "Arduino.h"
#include "pindefs_koala.h"

#define LED_RED A3
#define LED_YELLOW A4
#define LED_GREEN A5

// LED constructor
LEDKoala::LEDKoala() {
    this->red_state = false;
    this->yellow_state = false;
    this->green_state = false;

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
}

void LEDKoala::ctrl_LEDs(double vel, double deadband, bool enabled) {
    ctrl_red(vel, deadband, enabled);
    ctrl_yellow(vel, deadband, enabled);
    ctrl_green(vel, deadband, enabled);
}

void LEDKoala::ctrl_red(double vel, double deadband, bool enabled) {
    // turn red LED on if motor is stopped
    if ((vel > deadband * -1.0 && vel < deadband) && enabled) {
        this->red_state = true;
    } else {
        this->red_state = false;
    }

    if (this->red_state) {
        digitalWrite(LED_RED, HIGH);
    } else {
        digitalWrite(LED_RED, LOW);
    }
}

void LEDKoala::ctrl_yellow(double vel, double deadband, bool enabled) {
    // turn yellow LED on if motor is going backwards
    if ((vel < deadband * -1.0) && enabled) {
        this->yellow_state = true;
    } else {
        this->yellow_state = false;
    }

    if (this->yellow_state) {
        digitalWrite(LED_YELLOW, HIGH);
    } else {
        digitalWrite(LED_YELLOW, LOW);
    }
}

void LEDKoala::ctrl_green(double vel, double deadband, bool enabled) {
    // turn green LED on if motor moving forward
    if (vel > deadband && enabled) {
        this->green_state = true;
    } else {
        this->green_state = false;
    }

    if (this->green_state) {
        digitalWrite(LED_GREEN, HIGH);
    } else {
        digitalWrite(LED_GREEN, LOW);
    }
}

void LEDKoala::test_LEDs() {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_YELLOW, HIGH);

    delay(1000);

    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
}

void LEDKoala::setup_LEDs() {
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
}
