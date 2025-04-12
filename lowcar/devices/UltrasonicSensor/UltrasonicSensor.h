#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

#include <stdint.h>
#include "Device.h"
#include "defs.h"

class UltrasonicSensor : public Device {
  public:
    // Constructor
    UltrasonicSensor();

    virtual size_t device_read(uint8_t param, uint8_t* data_buf);
    virtual void device_enable();

    // virtual void device_enable() override;
    // virtual void device_reset() override;
    // virtual void device_actions() override;

  private:
    static uint8_t trigPin;
    static uint8_t echoPin;
};

#endif
