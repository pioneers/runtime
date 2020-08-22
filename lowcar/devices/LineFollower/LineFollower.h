#ifndef LINE_FOLLOWER_H
#define LINE_FOLLOWER_H

#include "Device.h"
#include "defs.h"

class LineFollower : public Device {
  public:
    // Simply calls generic Device constructor with device type and year
    LineFollower();

    // overridden functions from Device class; see descriptions in Device.h
    virtual size_t device_read(uint8_t param, uint8_t* data_buf);
    virtual void device_enable();

  private:
    // number of pins used for I/O for LineFollower
    const static int NUM_PINS;
    // pins that the line follower reads data from (defined in defs.h)
    const static uint8_t pins[];
};

#endif
