#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

#include "Device.h"
#include "defs.h"

class UltrasonicSensor : public Device {
  public:
    UltrasonicSensor();

    virtual size_t device_read(uint8_t param, uint8_t* data_buf) override;
    virtual size_t device_write(uint8_t param, uint8_t* data_buf) override;
    virtual void device_enable() override;
    virtual void device_reset() override;
    virtual void device_actions() override;

  private:
    static const uint8_t trigPin;
    static const uint8_t echoPin;
};

#endif
