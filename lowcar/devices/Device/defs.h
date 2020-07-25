#ifndef DEFS_H
#define DEFS_H

#include "Arduino.h"
#include <stdint.h>

#define PARAM_BITMAP_BYTES  4

#define MAX_PARAMS          32

//Maximum size of a message payload
//achieved with a DEVICE_WRITE/DEVICE_DATA of MAX_PARAMS of all floats
#define MAX_PAYLOAD_SIZE    (PARAM_BITMAP_BYTES + (MAX_PARAMS * sizeof(float)))

// Use these with uint8_t instead of `bool` with `true` and `false`
// This makes device_read() and device_write() cleaner when parsing on C
#define TRUE 1
#define FALSE 0

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
	LINE_FOLLOWER     = 0x01,
	//POTENTIOMETER     = 0x02,
	//ENCODER           = 0x03,
	BATTERY_BUZZER    = 0x04,
	//TEAM_FLAG         = 0x05,
	//GRIZZLY           = 0x06,
	SERVO_CONTROL     = 0x07,
	//LINEAR_ACTUATOR   = 0x08,
	//COLOR_SENSOR      = 0x09,
	//YOGI_BEAR         = 0x0A,
	RFID              = 0x0B,
	POLAR_BEAR        = 0x0C,
	KOALA_BEAR        = 0x0D,
	DUMMY_DEVICE      = 0x0E
	//DISTANCE_SENSOR   = 0x10,
	//METAL_DETECTOR    = 0x11,
	//EXAMPLE_DEVICE    = 0xFF,
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
