#include "LED.h"

// pins that control the red, yellow, and green LEDs
#define LED_RED		2
#define LED_YELLOW	3
#define LED_GREEN	4

// decides when the red LED is on
static void ctrl_RED(float duty_cycle) {
    if (duty_cycle < -0.5) {
        digitalWrite(LED_RED, HIGH);
    } else {
        digitalWrite(LED_RED, LOW);
    }
}

// decides when the yellow LED is on
static void ctrl_YELLOW(float duty_cycle) {
    if (duty_cycle > -0.5 && duty_cycle < 0.5) {
        digitalWrite(LED_YELLOW, HIGH);
    } else {
        digitalWrite(LED_YELLOW, LOW);
    }
}

// decides when the green LED is on
static void ctrl_GREEN(float duty_cycle) {
    if (duty_cycle >= 0.5) {
        digitalWrite(LED_GREEN, HIGH);
    } else {
        digitalWrite(LED_GREEN, LOW);
    }
}

// place all LED control functions in here
void ctrl_LEDs(float duty_cycle) {
    ctrl_RED(duty_cycle);
    ctrl_YELLOW(duty_cycle);
    ctrl_GREEN(duty_cycle);
}

// sets the output modes of the three LEDs to OUTPUT and blinks them to make sure they work
void setup_LEDs() {
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    
    digitalWrite(LED_GREEN,HIGH);
    digitalWrite(LED_RED,HIGH);
    digitalWrite(LED_YELLOW,HIGH);
    
    delay(1000);
    
    digitalWrite(LED_GREEN,LOW);
    digitalWrite(LED_RED,LOW);
    digitalWrite(LED_YELLOW,LOW);
}
