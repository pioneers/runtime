#include "GeneralSerial.h"

GeneralSerial::GeneralSerial(bool is_hardware_serial, HardwareSerial* hw_serial_port) {
    this->is_hardware_serial = is_hardware_serial;
    this->hw_serial_port = hw_serial_port;
}

int GeneralSerial::peek() {
    if (is_hardware_serial) {
        return this->hw_serial_port->peek();
    } else {
        return Serial.peek();
    }
}

int GeneralSerial::available() {
    if (is_hardware_serial) {
        return this->hw_serial_port->available();
    } else {
        return Serial.available();
    }
}

void GeneralSerial::begin(unsigned long baud) {
    if (is_hardware_serial) {
        this->hw_serial_port->begin(baud);
    } else {
        Serial.begin(baud);
    }
}

size_t GeneralSerial::write(const uint8_t byte) {
    if (is_hardware_serial) {
        return this->hw_serial_port->write(byte);
    } else {
        return Serial.write(byte);
    }
}

size_t GeneralSerial::write(const uint8_t* buffer, size_t size) {
    if (is_hardware_serial) {
        return this->hw_serial_port->write(buffer, size);
    } else {
        return Serial.write(buffer, size);
    }
}

int GeneralSerial::read() {
    if (is_hardware_serial) {
        return this->hw_serial_port->read();
    } else {
        return Serial.read();
    }
}