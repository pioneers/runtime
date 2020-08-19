#ifndef DEFS_H
#define DEFS_H

#include "Arduino.h"
#include <stdint.h>

#define PARAM_BITMAP_BYTES  4

#define MAX_PARAMS          32

// Maximum size of a message payload
// achieved with a DEVICE_WRITE/DEVICE_DATA of MAX_PARAMS of all floats
#define MAX_PAYLOAD_SIZE    (PARAM_BITMAP_BYTES + (MAX_PARAMS * sizeof(float)))

// Use these with uint8_t instead of `bool` with `true` and `false`
// This makes device_read() and device_write() cleaner when parsing on C
#define TRUE 1
#define FALSE 0

// identification for analog pins
enum class Analog : uint8_t {
    IO0 = A0,
    IO1 = A1,
    IO2 = A2,
    IO3 = A3
};

// identification for digital pins
enum class Digital : uint8_t {
    IO4 = 2,
    IO5 = 3,
    IO6 = 6,
    IO7 = 9,
    IO8 = 10,
    IO9 = 11
};

/* The types of messages */
enum class MessageID : uint8_t {
    NOP                     = 0x00, // Dummy message
    PING                    = 0x01, // To lowcar
    ACKNOWLEDGEMENT         = 0x02, // To dev handler
    SUBSCRIPTION_REQUEST    = 0x03, // To lowcar
    DEVICE_WRITE            = 0x04, // To lowcar
    DEVICE_DATA             = 0x05, // To dev handler
    LOG                     = 0x06  // To dev handler
};

// identification for device types
enum class DeviceType : uint16_t {
    DUMMY_DEVICE      = 0x00,
    LIMIT_SWITCH      = 0x01,
    LINE_FOLLOWER     = 0x02,
    BATTERY_BUZZER    = 0x03,
    SERVO_CONTROL     = 0x04,
    POLAR_BEAR        = 0x05,
    KOALA_BEAR        = 0x06
    // DISTANCE_SENSOR   = 0x07 Uncomment when implemented
};

// identification for resulting status types
enum class Status {
    SUCCESS,
    PROCESS_ERROR,
    MALFORMED_DATA,
    NO_DATA
};

// decoded lowcar packet
typedef struct {
    MessageID message_id;
    uint8_t payload_length;
    uint8_t payload[MAX_PAYLOAD_SIZE];
} message_t;

// unique id struct for a specific device
typedef struct {
    DeviceType type;
    uint8_t year;
    uint64_t uid;
} dev_id_t;

#endif
