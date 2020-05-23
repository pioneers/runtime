#include <stdio.h>
#include "devices.h"
#include <stdint.h>
#include <string.h>
#include "shm_wrapper.h"

/* The length of the largest payload in bytes, which may be reached for DeviceWrite and DeviceData message types. */
#define MAX_PAYLOAD_SIZE 132
// An array of pointers to official devices
dev_t* DEVICES[14];

/* A struct to hold the 88-bit identifier for a device
 * 16 bits: Device Type (ex: LimitSwitch)
 *  8 bits: Year (ex: 0x00 for 2015-2016 season)
 * 64 bits: "UID": A random hash for a specific physical device within a specific type/year
 * See page 9 of book.pdf for further details
*/

/************** BTW this is also defined in shm_wrapper so we can delete this later when we include shm_wrapper *********/
typedef struct dev_id {
    uint16_t type;
    uint8_t year;
    uint64_t uid;
} dev_id_t;

// The types of message
enum packet_type: uint8_t {
    Ping =  0x10,
    SubscriptionRequest = 0x11,
    SubscriptionResponse = 0x12,
    DeviceRead = 0x13,
    DeviceWrite = 0x14,
    DeviceData = 0x15,
    Disable = 0x16,
    HeartBeatRequest = 0x17,
    HeartBeatResponse = 0x18,
    Print = 0x19, // A packet used for print/debugging purposes
    Error = 0xFF,
};

typedef struct message_t {
    packet_type    message_id;
    uint8_t*       payload;             // Array of 8-bit integers
    uint8_t        payload_length;      // The current number of 8-bit integers in payload
    uint8_t        max_payload_length;  // The maximum length of the payload for the specific message_id
} message_t;

/* Utility functions for breaking apart 88-bit device info */
uint16_t get_device_type(dev_id_t id);
uint8_t get_year(dev_id_t id);
uint64_t get_uid(dev_id_t id);

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
message_t* make_print();
message_t* make_error();
void destroy_message(message_t* message);

/* Functions to split up a message (string of bits) into individual components */
//subscription_response_t parse_subscription_response(/* 120 bit message */);
//param_value_t** decode_device_write(/*120 bit message*/, /*device_id*/);
//param_value_t** parse_device_data(/*120 bit message*/, /*device_id*/);
//message_t* parse_bytes(/*msg_bytes*/);

/* Core read/write */
message_t* read(/* */);

/* Checksum and Cobs encode/decode */
char checksum(char* data, int len);
 /* Append data to end of payload of MSG */
int append_payload(message_t *msg, uint8_t *data, uint8_t length);
/* Cobs encode/decode */
size_t cobs_encode(uint8_t *dst, const uint8_t *src, size_t src_len);
size_t cobs_decode(uint8_t *dst, const uint8_t *src, size_t src_len);

/* Utility functions */
/* Converts a device name to the 16-bit type. */
uint16_t device_name_to_type(char* dev_name);
/* Converts a device type to its name */
char* device_type_to_name(uint16_t dev_type);
/* Returns an array of param name strings given a device type */
void all_params_for_device_type(uint16_t dev_type, char* param_names[]);
/* Check if PARAM of device type is readable */
int readable(uint16_t dev_type, char* param_name);
/* Check if PARAM of device type is writable */
int writable(uint16_t dev_type, char* param_name);
/* Return the data type of the device type's parameter */
char* param_type(uint16_t dev_type, char* param_name);
