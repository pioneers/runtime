#include <stdio.h>
#include "devices.h"
#include <stdint.h>
#include <string.h>

// An array of pointers to official devices
dev* DEVICES[14];

// The types of message
enum packet_type {
    Ping =  0x10,
    SubscriptionRequest = 0x11,
    SubscriptionResponse = 0x12,
    DeviceRead = 0x13,
    DeviceWrite = 0x14,
    DeviceData = 0x15,
    Disable = 0x16,
    HeartBeatRequest = 0x17,
    HeartBeatResponse = 0x18,
    Error = 0xFF,
};

typedef struct message_t {
    int            message_id;
    int*           payload;
    int            length;
} message_t;

typedef struct subscription_response_t {
    uint16_t params;
    uint16_t delay;
    uint64_t uid;
} subscription_response_t;

typedef struct param_value_t {
    int param;
    int value;
} param_value_t;

/* A struct to hold the 88-bit identifier for a device
 * 16 bits: Device Type (ex: LimitSwitch)
 *  8 bits: Year (ex: 0x00 for 2015-2016 season)
 * 64 bits: "UID": A random hash for a specific physical device within a specific type/year
 * See page 9 of book.pdf for further details
*/
typedef struct dev_id_t {
    uint16_t type;
    uint8_t year;
    uint64_t uid;
} dev_id_t;

/* Utility functions for breaking apart 88-bit device info */
uint16_t get_device_type(dev_id_t id);
uint8_t get_year(dev_id_t id);
uint64_t get_uid(dev_id_t id);


char checksum(char* data, int len);
void send(/* Connection */);
// encode_params
// decode_params

/* Functions to create the appropriate message_t struct */
message_t* make_ping();
message_t* make_disable();
message_t* make_heartbeat_request();
message_t* make_heartbeat_response();
message_t* make_subscription_request();
message_t* make_subscription_response();
message_t* make_device_read();
message_t* make_device_write();
message_t* make_device_data();
message_t* make_error();

/* Functions to split up a message (string of bits) into individual components */
//subscription_response_t parse_subscription_response(/* 120 bit message */);
//param_value_t** decode_device_write(/*120 bit message*/, /*device_id*/);
//param_value_t** parse_device_data(/*120 bit message*/, /*device_id*/);
//message_t* parse_bytes(/*msg_bytes*/);

/* Core read/write */
message_t* read(/* */);

/* Checksum and Cobs encode/decode */
char* cobs_encode(/* data */);
char* cobs_decode(/* data */);

/* Utility functions */
/* Converts a device name to the 16-bit type. */
uint16_t device_name_to_type(char* dev_name);

char* device_type_to_name(uint16_t dev_type);
/* Returns an array of param name strings given a device type */
char** all_params_for_device_type(uint16_t dev_type);
/* Check if PARAM of device type is readable */
int readable(uint16_t dev_type, char* param_name);
/* Check if PARAM of device type is writable */
int writable(uint16_t dev_type, char* param_name);
/* Return the data type of the device type's parameter */
char* param_type(uint16_t dev_type, char* param_name);
