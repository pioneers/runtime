/*
 * A class used as a wrapper for both Serial and Serial1, Serial2, etc.
 * On Arduino Leonardos, the Serial port is not a real USB port, it's a
 * virtual USB, so we cannot use the HardwareSerial class to wrap both.
 */

#ifndef GENERAL_SERIAL_H
#define GENERAL_SERIAL_H

#include <Arduino.h>

class GeneralSerial : public Stream {
    public:
        /*
         * Constructor of a GeneralSerial object
         *    Arguments:
         *      is_hardware_serial: False by default (use Serial); set to True for devices that use SerialX pins
         *      hw_serial_prt: Unused (NULL) by default; when is_hardware_serial == True, specify which SerialX port to use
         */
        GeneralSerial(bool is_hardware_serial = false, HardwareSerial *hw_serial_port = NULL);
        
        /*
         * All of these are taken from the Serial class; depending on whether the device is supposed to be
         * Serial or Serial1/2 etc., these functions will just call the corresponding function and return the result
         */
        virtual int peek();
        virtual int available();
        virtual void begin(unsigned long baud);
        virtual size_t write(const uint8_t byte);
        virtual size_t write(const uint8_t *buffer, size_t size);
        virtual int read();
        
    private:
        bool is_hardware_serial;
        HardwareSerial *hw_serial_port;
};

#endif