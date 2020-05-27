#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "devices.h"
#include "../runtime_util.h"
#include "../shm_wrapper/shm_wrapper.h"

/* The length of the largest payload in bytes, which may be reached for DeviceWrite and DeviceData message types. */
#define MAX_PAYLOAD_SIZE 132

/* A struct to hold the 88-bit identifier for a device
 * 16 bits: Device Type (ex: LimitSwitch)
 *  8 bits: Year (ex: 0x00 for 2015-2016 season)
 * 64 bits: "UID": A random hash for a specific physical device within a specific type/year
 * See page 9 of book.pdf for further details
*/

// The types of message
typedef enum packet_type {
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
} packet_type;

typedef struct message {
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

#endif
