#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include "Device.h"
#include "defs.h"
#include <Servo.h>

class ServoControl : public Device {
public:
    /**
     * Initializes servos and disables them
     */
    ServoControl();

    // Overridden functions
    // Comments/descriptions can be found in source file and in Device.h
    virtual size_t device_read(uint8_t param, uint8_t *data_buf);

    /**
     * Updates provided servo position.
     * Updates pulse width associated with specified servo
     * Attaches servo at param to corresponding pin if not attached yet
     */
    virtual size_t device_write(uint8_t param, uint8_t *data_buf);
    virtual void device_disable(); // calls helper disableAll() to detach all servos

private:
    // class constants and variables
    const static int NUM_SERVOS, SERVO_0, SERVO_1, SERVO_CENTER, SERVO_RANGE;
    const static uint8_t pins[];

    float *positions;

    // detaches all servos
    void disable_all();
};

#endif
