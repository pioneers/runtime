#ifndef LED_H
#define LED_H

#include "Arduino.h"
#include "defs.h"

//function prototypes
void ctrl_LEDs(float duty_cycle, float deadband);
void test_LEDs();
void setup_LEDs();

#endif /* LED_H */
