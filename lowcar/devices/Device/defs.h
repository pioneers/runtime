#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include "Arduino.h"

// The maximum number of parameters for a lowcar device
#define MAX_PARAMS 32

// Number of milliseconds between sending data to Runtime
#define DATA_INTERVAL_MS 1

// The size of the param bitmap used in various messages (8 bits in a byte)
#define PARAM_BITMAP_BYTES (MAX_PARAMS / 8)

// Maximum size of a message payload
// achieved with a DEVICE_WRITE/DEVICE_DATA of MAX_PARAMS of all floats
#define MAX_PAYLOAD_SIZE (PARAM_BITMAP_BYTES + (MAX_PARAMS * sizeof(float)))

// Use these with uint8_t instead of `bool` with `true` and `false`
// This makes device_read() and device_write() cleaner when parsing on C
#define TRUE 1
#define FALSE 0

// identification for analog pins
enum Analog : uint8_t {
    IO0 = A0,
    IO1 = A1,
    IO2 = A2,
    IO3 = A3
};

// identification for digital pins
enum Digital : uint8_t {
    IO4 = 2,
    IO5 = 3,
    IO6 = 6,
    IO7 = 9,
    IO8 = 10,
    IO9 = 11
};

/* The types of messages */
enum class MessageID : uint8_t {
    NOP = 0x00,              // Dummy message
    DEVICE_PING = 0x01,      // To lowcar
    ACKNOWLEDGEMENT = 0x02,  // To dev handler
    DEVICE_WRITE = 0x03,     // To lowcar
    DEVICE_DATA = 0x04,      // To dev handler
    LOG = 0x05               // To dev handler
};

// identification for device types
enum class DeviceType : uint8_t {
    DUMMY_DEVICE = 0x00,
    LIMIT_SWITCH = 0x01,
    LINE_FOLLOWER = 0x02,
    BATTERY_BUZZER = 0x03,
    SERVO_CONTROL = 0x04,
    POLAR_BEAR = 0x05,
    KOALA_BEAR = 0x06,
    PDB = 0x07
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
