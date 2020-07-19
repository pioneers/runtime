#ifndef DEFS_H
#define DEFS_H

#include "Arduino.h"
#include <stdint.h>

//Maximum size of a message payload
#define MAX_PAYLOAD_SIZE    132

#define PARAM_BITMAP_BYTES  4

//identification for analog pins
enum class Analog : uint8_t {
  IO0 = A0,
  IO1 = A1,
  IO2 = A2,
  IO3 = A3
};

//identification for digital pins
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
    NOP                     = 0x00,  // Dummy message
    PING                    = 0x01, // Bidirectional
    ACKNOWLEDGEMENT         = 0x02, // To dev handler
    SUBSCRIPTION_REQUEST    = 0x03, // To lowcar
    DEVICE_WRITE            = 0x04, // To lowcar
    DEVICE_DATA             = 0x05, // To dev handler
    LOG                     = 0x06  // To dev handler
};

//identification for device types
enum class DeviceType : uint16_t {
  LIMIT_SWITCH      = 0x00,
  POLAR_BEAR        = 0x0C,
  LINE_FOLLOWER     = 0x01,
  BATTERY_BUZZER    = 0x04,
  TEAM_FLAG         = 0x05,
  RFID              = 0x0B,
  SERVO_CONTROL     = 0x07,
  COLOR_SENSOR      = 0x09,
  KOALA_BEAR        = 0x0D,
  EXAMPLE_DEVICE    = 0xFF,
  DUMMY_DEVICE      = 0x0E
};

//identification for resulting status types
enum class Status {
  SUCCESS,
  PROCESS_ERROR,
  MALFORMED_DATA,
  NO_DATA
};

//decoded lowcar packet
typedef struct {
  MessageID message_id;
  uint8_t payload_length;
  uint8_t payload[MAX_PAYLOAD_SIZE];
} message_t;

//unique id struct for a specific device
typedef struct {
  DeviceType type;
  uint8_t year;
  uint64_t uid;
} dev_id_t;

#endif
