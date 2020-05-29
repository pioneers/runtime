#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>                         // strcmp
#include "devices.h"
#include "../runtime_util/runtime_util.h"
#include "../shm_wrapper/shm_wrapper.h"

/* The length of the largest payload in bytes, which may be reached for DeviceWrite and DeviceData message types. */
#define MAX_PAYLOAD_SIZE 132

/* The types of messages */
typedef enum packet_type {
    Ping = 0x10,
    SubscriptionRequest = 0x11,
    SubscriptionResponse = 0x12,
    DeviceRead = 0x13,
    DeviceWrite = 0x14,
    DeviceData = 0x15,
    Disable = 0x16,
    HeartBeatRequest = 0x17,
    HeartBeatResponse = 0x18,
    Log = 0x19, // A packet used for print/debugging purposes
    Error = 0xFF,
} packet_type;

/* The types of error codes a device can send */
typedef enum error_code {
    UnexpectedDelimiter = 0xFD,
    CheckumError = 0xFE,
    GenericError = 0xFF
} error_code_t;

/* A struct defining a message to be sent over serial */
typedef struct message {
    packet_type    message_id;
    uint8_t*       payload;             // Array of 8-bit integers
    uint8_t        payload_length;      // The current number of 8-bit integers in payload
    uint8_t        max_payload_length;  // The maximum length of the payload for the specific message_id
} message_t;

/*
 * Utility functions for breaking apart the 88-bit device info (dev_id_t)
 */
uint16_t get_device_type(dev_id_t id);
uint8_t get_year(dev_id_t id);
uint64_t get_uid(dev_id_t id);

/******************************************************************************************
 *                              MESSAGE CONSTRUCTORS
 ******************************************************************************************/
/*
 * A message that pings a device
 * Payload: empty
 * Direction: dev_handler --> device
 */
message_t* make_ping();

/*
 * A message to disable a device
 * Payload: empty
 * Direction: dev_handler --> device
 */
message_t* make_disable();

/*
 * A message to ask the receiver to immediately send back a HeartBeatResponse
 * heartbeat_id: An optional character to send in the payload.
 *  May be useful for keeping track of individual heartbeats
 * Payload: 8-bit heartbeat_id
 * Direction: dev_handler <--> device
 */
message_t* make_heartbeat_request(char heartbeat_id);

/*
 * A message to respond to a HeartBeatRequest
 * Should not be sent otherwise.
 * heartbeat_id: An optional character to send in the payload.
 *  May be useful for keeping track of individual heartbeats
 *
 * Payload: 8-bit heartbeat_id
 * Direction: dev_handler <--> device
 */
message_t* make_heartbeat_response(char heartbeat_id);

/*
 * A message to request parameter data from a device at an interval
 * device_id: The device to request data from
 * param_names: An array of the parameter names to request data about
 * len: The length of param_names
 * delay: The number of microseconds to wait between each response
 *
 * Payload: 32-bit param bit-mask, 16-bit delay
 * Direction: dev_handler --> device
 */
message_t* make_subscription_request(dev_id_t device_id, char* param_names[], uint8_t len, uint16_t delay);

/*
 * A message in response to a SubscriptionRequest.
 * device_id: The device to request data from
 * param_names: An array of the parameter names to request data about
 * len: The length of param_names
 * delay: The number of microseconds to wait between each response
 *
 * Payload: 32-bit param bit-mask, 16-bit delay, 88-bit device identifier
 * Direction: device --> dev_handler
 */
message_t* make_subscription_response(dev_id_t* device_id, char* param_names[], uint8_t len, uint16_t delay);

/*
 * A message to request parameter data from a device (a single request, unlike a subscription)
 * The device is expected to respond with a DeviceData message with the data of the readable parameters.
 * device_id: The device to request data from
 * param_names: An array of the parameter names to request data about
 * len: The length of param_names
 *
 * Payload: 32-bit param mask
 * Direction: dev_handler --> device
 */
message_t* make_device_read(dev_id_t* device_id, char* param_names[], uint8_t len);

/*
 * A message to write data to the specified parameters of a device
 * The device is expected to respond with a DeviceData message confirming the new data of the writable parameters.
 * device_id: The device to request data from
 * param_names: An array of the parameter names to request data about
 * len: The length of param_names
 *
 * Payload: 32-bit param mask, 32-bits for parameter 0 data, 32-bits for parameter 1 data, . . ., 32-bits for parameter 31 data
 * Direction: dev_handler --> device
 */
message_t* make_device_write(dev_id_t device_id, param_val_t* param_values[], int len);

/*
 * A message that a device responds with to DeviceRead, DeviceWrite, or SubscriptionRequest
 *
 * Payload: 32-bit param mask, 32-bits for parameter 0 data, 32-bits for parameter 1 data, . . ., 32-bits for parameter 31 data
 * Direction: device --> dev_handler
 */
message_t* make_device_data();

/*
 * A message that a device sends that contains debugging information to be logged.
 * data: The message to be encoded and sent (NULL-TERMINATED STRING))
 *
 * Payload: 1056 bits
 * Direction: device --> dev_handler
 */
message_t* make_log(char* data);

/*
 * A message that a device sends indicating that an error has occurred
 * error_code: The error code indicating what type of error occurred
 *
 * Payload: 8-bit error_code
 * Direction: device --> dev_handler
 */
message_t* make_error(uint8_t error_code);

/* Frees the memory allocated for the message struct and its payload. */
void destroy_message(message_t* message);

/******************************************************************************************
 *                          Building, encoding, and decoding data
 ******************************************************************************************/
/*
 * Appends data to the end of a message's payload
 * msg: The message whose payload is to be appended to
 * data: The data to be appended to the payload
 * length: The length of data in bytes
 * returns: 0 if successful. -1 otherwise due to overwriting
 */
int append_payload(message_t *msg, uint8_t *data, uint8_t length);

/*
 * Computes the checksum of data
 * data: A character array
 * len: the length of data
 * returns: The checksum
 */
char checksum(char* data, int len);

/*
 * Cobs encodes data into a buffer
 * src: The data to be encoded
 * dst: The buffer to write the encoded data to
 * src_len: The size of the source data
 * return: The size of the encoded data
 */
size_t cobs_encode(uint8_t *dst, const uint8_t *src, size_t src_len);

/*
 * Cobs decodes data into a buffer
 * src: The data to be decoded
 * dst: The buffer to write the decoded data to
 * src_len: The size of the source data
 * return: The size of the decoded data
 */
size_t cobs_decode(uint8_t *dst, const uint8_t *src, size_t src_len);

/*
 * Converts an array of parameter names to a 32-bit mask.
 * device_type: The device that include the specified params
 * params: Array of names for which the bit will be on.
 * len: The length of params
 * return: 32-bit mask. bit i is on if the device's parameter i is in PARAMS. Otherwise bit i is off.
 */
uint32_t encode_params(uint16_t device_type, char** params, uint8_t len);

/*
 * Potentially added later:
 * toBytes(message_t), parseMessage(bytes)
 * sendMessage(), read() from serial to get mesage
 * decode_params
 * parse_subscription_response: Break down sub_response received into its parts
 * parse_device_data: Break down DeviceData packet into param, value pairs
 *
 */
#endif
