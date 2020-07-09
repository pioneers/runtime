/**
 * Functions to build, encode, decode, serialize, and parse messages
 * Each message, when serialized is in the following format:
 * [delimiter][Length of cobs encoded message][Cobs encoded message]
 * The cobs encoded message, when decoded is in the following format:
 * [message id][payload length][payload][checksum]
 * The size of each section are defined as constants in message.c
 *
 * Serves as a utility API for DEV_HANDLER
 */
#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h> // strcmp
#include "../runtime_util/runtime_util.h"
#include "../shm_wrapper/shm_wrapper.h"

// The size in bytes of the message delimiter
#define DELIMITER_SIZE 1
// The size in bytes of the section specifying the length of the cobs encoded message
#define COBS_LENGTH_SIZE 1
// The size in bytes of the section specifying the type of message being encoded
#define MESSAGE_ID_SIZE 1
// The size in bytes of the section specifying the length of the payload in the message
#define PAYLOAD_LENGTH_SIZE 1
// The size in bytes of the param bitmap in SubscriptionRequest, SubscriptionResponse, DeviceRead, DeviceWrite and DeviceData payloads
#define BITMAP_SIZE (MAX_PARAMS/8)
// The size in bytes of the section specifying the delay for SubscriptionRequest and SubscriptionResponse
#define DELAY_SIZE 2
// The size in bytes of the section specifying the device id
#define DEVICE_ID_SIZE 11
// The size in bytes of the section specifying the checksum of the message id, the payload length, and the payload itself
#define CHECKSUM_SIZE 1
// The length of the largest payload in bytes, which may be reached for DeviceWrite and DeviceData message types.
#define MAX_PAYLOAD_SIZE (BITMAP_SIZE+(MAX_PARAMS*sizeof(float))) // Bitmap + Each param (may be floats)

/* The types of messages */
typedef enum {
    PING                    = 0x01, // Bidirectional
    ACKNOWLEDGEMENT         = 0x02, // To dev handler
    SUBSCRIPTION_REQUEST    = 0x03, // To lowcar
    DEVICE_WRITE            = 0x04, // To lowcar
    DEVICE_DATA             = 0x05, // To dev handler
    LOG                     = 0x06  // To dev handler
} message_id_t;

/* A struct defining a message to be sent over serial */
typedef struct message {
    message_id_t   message_id;
    uint8_t*       payload;             // Array of 8-bit integers
    uint8_t        payload_length;      // The current number of 8-bit integers in payload
    uint8_t        max_payload_length;  // The maximum length of the payload for the specific message_id
} message_t;

/* Prints the byte array DATA of length LEN in hex */
void print_bytes(uint8_t* data, int len);

/******************************************************************************************
 *                              MESSAGE CONSTRUCTORS                                      *
 *   The message returned from these MUST be deallocated memory using destroy_message()   *
 *   Message constructors for only device --> dev_handler included for completion's sake  *
 ******************************************************************************************/

/*
 * Utility function to allocate memory for an empty message
 * To be used with parse_message()
 * payload_size: The size of memory of allocate for the payload
 */
message_t* make_empty(ssize_t payload_size);

/*
 * A message that pings a device
 * Payload: empty
 */
message_t* make_ping();

/*
 * A message to request parameter data from a device at an interval
 * device_id: The device to request data from
 * param_names: An array of the parameter names to request data about
 * len: The length of param_names
 * delay: The number of microseconds to wait between each response
 *
 * Payload: 32-bit param bit-mask, 16-bit delay
 */
message_t* make_subscription_request(dev_id_t* device_id, char* param_names[], uint8_t len, uint16_t delay);

/*
 * A message to write data to the specified writable parameters of a device
 * device_id: The id of the device to write data to
 * param_bitmap: The 32-bit param bitmap indicating which parameters will be written to
 * param_values: An array of the parameter values. If i-th bit in the bitmap is on, its value is in the i-th index.
 *
 * Payload: 32-bit param mask, each of the param_values specified (number of bytes depends on the parameter type)
 */
message_t* make_device_write(dev_id_t* device_id, uint32_t param_bitmap, param_val_t param_values[]);

/*
 * Frees the memory allocated for the message struct and its payload.
 * message: The messsage to have its memory deallocated.
 */
void destroy_message(message_t* message);

/******************************************************************************************
 *                          Serializing and Parsing Messages                              *
 ******************************************************************************************/

/*
 * Converts an array of parameter names to a 32-bit mask.
 * device_type: The device that include the specified params
 * params: Array of names for which the bit will be on.
 * len: The length of params
 * return: 32-bit mask. bit i is on if the device's parameter i is in PARAMS. Otherwise bit i is off.
 */
uint32_t encode_params(uint16_t device_type, char** params, uint8_t len);

/*
 * Calculates the largest length possible of a cobs-encoded msg
 * Used when allocating a buffer to parse a message into via parse_message()
 * msg: The msg to be serialized to a byte array
 * return: the maximum length
 */
int calc_max_cobs_msg_length(message_t* msg);

/*
 * Serializes msg into a cobs-encoded byte array
 * msg: the message to serialize
 * data: empty buffer to be filled with the cobs-encoded message
 * len: the length of DATA. Should be at least calc_max_cobs_msg_length(msg)
 * return: The length of the serialized message. -1 if len is too small (less than calc_max_cobs_msg_length)
 */
int message_to_bytes(message_t* msg, uint8_t data[], int len);

/*
 * Cobs decodes a byte array and populates a message_t*, filling up the fields of msg_to_fill
 * The structure of the byte array is expected to be:
 *      8-bit MessageID + 8-Bit PayloadLength + Payload + 8-Bit Checksum
 * data: A byte array containing a cobs encoded message. data[0] should be the delimiter. data[1] should be cobs_len
 * empty_msg: A message to be populated. Payload must be properly allocated memory. Use make_empty()
 * return: 0 if successful parsing (correct checksum). 1 Otherwise
 */
int parse_message(uint8_t data[], message_t* empty_msg);

/*
 * Reads the parameter values from a DeviceData/DeviceWrite message into vals
 * dev_type: The device type that the message is sent from/to
 * dev_data: The DeviceData/DeviceWrite message
 * vals: An array of param_val_t structs to be populated with the values from the message.
 *  NOTE: The length of vals MUST be at LEAST the number of params sent in the DeviceData message
 *  Tip: Allocate MAX_PARAMS param_val_t structs to guarantee this
 */
void parse_device_data(uint16_t dev_type, message_t* dev_data, param_val_t vals[]);

/*
 * Potentially added later:
 * decode_params
 */
#endif
