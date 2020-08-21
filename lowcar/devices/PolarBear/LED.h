#ifndef LED_H
#define LED_H

#include "Arduino.h"
#include "defs.h"

/**
 * Updates all LEDs based on current duty_cycle
 * Arguments:
 *    duty_cycle: The motor controller duty_cycle
 *    deadband: The motor controller deadband
 */
void ctrl_LEDs(float duty_cycle, float deadband);

/**
 * Sets the output modes of the three LEDs to OUTPUT
 * Blinks them to make sure they work
 */
void setup_LEDs();

#endif /* LED_H */
