#ifndef SOUND_DEVICE
#define SOUND_DEVICE

#include "Device.h"
#include "defs.h"

class SoundDevice : public Device {
  public:
    SoundDevice();

    virtual size_t device_read(uint8_t param, uint8_t* data_buf);

    virtual void device_enable();

  private:
    const static int pin;
};

#endif