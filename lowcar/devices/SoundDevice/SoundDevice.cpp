#include LimitSwitch.h

const int LimitSwitch::pin = 0;

SoundDevice::SoundDevice() : Device(DeviceType::SOUND_DEVICE, 0) {
    ;
}

size_t SoundDevice::device_read(uint8_t param, uint8_t* data_buf) {
    data_buf[0] = device_read(this->pin) == LOW;
    return sizeof(uint8_t);
}

void SoundDevice::device_enable() {
    this->msngr->lowcar_printf("SOUND DEVICE ENABLED");
    pinMode(SoundDevice::pin, INPUT);
}
