#ifndef KOALABEAR_H
#define KOALABEAR_H

#include <SPI.h>

#include "Device.h"
#include "PID.h"
#include "LEDKoala.h"
#include "pindefs_koala.h"
#include "defs.h"

class KoalaBear : public Device {
public:
    KoalaBear ();
    virtual size_t device_read(uint8_t param, uint8_t *data_buf);
    virtual size_t device_write(uint8_t param, uint8_t *data_buf);
    virtual void device_enable();
    virtual void device_disable();
    virtual void device_actions();

private:
    float target_speed_a, target_speed_b; // target speeds of motors
    float deadband_a, deadband_b;         // deadbands of motors
    uint8_t pid_enabled_a, pid_enabled_b; // whether or not motors are enabled
    volatile uint32_t enc_a, enc_b;       // encoder values of motors
    PID *pid_a, *pid_b;                   // PID controllers for motors
    LEDKoala *led;                        // for controlling the KoalaBear LED
    unsigned long prev_led_time;
    int curr_led_mtr;
    
    void drive(float target, uint8_t mtr);
    void write_current_lim();
    void read_current_lim();
    void electrical_setup();
    
    //static void handle_enc_a_tick();
    //static void handle_enc_b_tick();
};

#endif
