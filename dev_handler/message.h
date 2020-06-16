#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h> // strcmp
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
// typedef enum error_code {
//     UnexpectedDelimiter = 0xFD,
//     CheckumError = 0xFE,
//     GenericError = 0xFF
// } error_code_t;

/* A struct defining a message to be sent over serial */
typedef struct message {
    packet_type    message_id;
    uint8_t*       payload;             // Array of 8-bit integers
    uint8_t        payload_length;      // The current number of 8-bit integers in payload
    uint8_t        max_payload_length;  // The maximum length of the payload for the specific message_id
} message_t;

/*
 * Private utility function to calculate the size of the payload needed
 * for a DeviceWrite/DeviceData message.
 * device_type: The type of device (Ex: 2 for Potentiometer)
 * param_bitmap: 32-bit param bit map. The i-th bit indicates whether param i will be transmitted in the message
 * return: The size of the payload
 */
// ssize_t device_data_payload_size(uint16_t device_type, uint32_t param_bitmap);

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
message_t* make_subscription_request(dev_id_t* device_id, char* param_names[], uint8_t len, uint16_t delay);

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
// message_t* make_subscription_response(dev_id_t* device_id, char* param_names[], uint8_t len, uint16_t delay);

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
 * A message to write data to the specified writable parameters of a device
 * The device is expected to respond with a DeviceData message confirming the new data of the writable parameters.
 * device_id: The id of the device to write data to
 * param_bitmap: The 32-bit param bitmap indicating which parameters will be written to
 * param_values: An array of the parameter values. If i-th bit in the bitmap is on, its value is in the i-th index.
 *
 * Payload: 32-bit param mask, each of the param_values specified (number of bytes depends on the parameter type)
 * Direction: dev_handler --> device
 */
message_t* make_device_write(dev_id_t* device_id, uint32_t param_bitmap, param_val_t param_values[]);

/*
 * A message that a device responds with to DeviceRead, DeviceWrite, or SubscriptionRequest
 *
 * Payload: 32-bit param mask, each of the param_values specified (number of bytes depends on the parameter type)
 * Direction: device --> dev_handler
 */
// message_t* make_device_data(dev_id_t* device_id, uint32_t param_bitmap, param_val_t param_values[]);

/*
 * A message that a device sends that contains debugging information to be logged.
 * data: The message to be encoded and sent (NULL-TERMINATED STRING))
 *
 * Payload: 1056 bits
 * Direction: device --> dev_handler
 * return: NULL if data is too large (over 132 bytes)
 */
// message_t* make_log(char* data);

/*
 * A message that a device sends indicating that an error has occurred
 * error_code: The error code indicating what type of error occurred
 *
 * Payload: 8-bit error_code
 * Direction: device --> dev_handler
 */
// message_t* make_error(uint8_t error_code);

/*
 * Frees the memory allocated for the message struct and its payload.
 * message: The messsage to have its memory deallocated.
 */
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
 * side-effect: increments msg->payload_length by length
 */
int append_payload(message_t *msg, uint8_t *data, uint8_t length);

/*
 * Computes the checksum of data
 * data: An array
 * len: the length of data
 * returns: The checksum
 */
uint8_t checksum(uint8_t* data, int len);

/*
 * Cobs encodes data into a buffer
 * src: The data to be encoded
 * dst: The buffer to write the encoded data to
 * src_len: The size of the source data
 * return: The size of the encoded data
 */
ssize_t cobs_encode(uint8_t *dst, const uint8_t *src, ssize_t src_len);

/*
 * Cobs decodes data into a buffer
 * src: The data to be decoded
 * dst: The buffer to write the decoded data to
 * src_len: The size of the source data
 * return: The size of the decoded data
 */
ssize_t cobs_decode(uint8_t *dst, const uint8_t *src, ssize_t src_len);

/*
 * Converts an array of parameter names to a 32-bit mask.
 * device_type: The device that include the specified params
 * params: Array of names for which the bit will be on.
 * len: The length of params
 * return: 32-bit mask. bit i is on if the device's parameter i is in PARAMS. Otherwise bit i is off.
 */
// uint32_t encode_params(uint16_t device_type, char** params, uint8_t len);

/*
 * Calculates the largest length possible of a cobs-encoded msg
 * msg: The msg to be serialized to a byte array
 * return: the maximum length
 */
int calc_max_cobs_msg_length(message_t* msg);

/*
 * Serializes msg into a byte array
 * msg: the message to serialize
 * data: empty buffer to be filled
 * len: the length of DATA. Should be at least calc_max_cobs_msg_length(msg)
 * return: The length of the serialized message. -1 if len is too small (less than calc_max_cobs_msg_length)
 */
int message_to_bytes(message_t* msg, uint8_t data[], int len);

/*
 * Converts a byte array into a message_t*, filling up the fields of msg_to_fill
 * The structure of the byte array:
 *      8-bit MessageID + 8-Bit PayloadLength + Payload + 8-Bit Checksum
 * data: A byte array containing a message
 * empty_msg: A message to be populated. Payload must be properly allocated memory. Use make_empty()
 * return: 0 if successful parsing (correct checksum). 1 Otherwise
 */
int parse_message(uint8_t data[], message_t* empty_msg);

/*
 * Reads the parameter values from a DeviceData/DeviceWrite message into vals
 * dev_type: The device type that the message is sent from/to
 * dev_data: The DeviceData/DeviceWrite message
 * vals: An array of param_val_t structs to be populated with the values from the message. Use make_empty_param_values;
 *  NOTE: The length of vals MUST be equal to the number of received values (make_empty_param_values should handle this)
 */
void parse_device_data(uint16_t dev_type, message_t* dev_data, param_val_t* vals[]);

/*
 * Utility function to allocate enough memory for a buffer to be used in parse_device_data
 * Use destroy_param_values to free vals
 * param_bitmap: The 32-bit param_bitmap for which each i-th bit requires a param_val_struct to be allocated
 * return: An array of pointers to param_val_t structs. The length is equal to the number of on bits in param_bitmap
 */
// param_val_t** make_empty_param_values(uint32_t param_bitmap);

/*
 * Utility function to free the param_val_t structs in an array.
 * To be used as a complement to make_empty_param_values
 * param_bitmap: 32-bit param_bitmap indicating which parameters are in VALS
 * vals: Array of param_val_t* to be freed. The length is equal to the number of on bits in param_bitmap
 */
// void destroy_param_values(uint32_t param_bitmap, param_val_t* vals[]);

/*
 * Potentially added later:
 * sendMessage(), read() from serial to get message         [Likely to be put in dev_handler]
 * decode_params
 * parse_subscription_response: Break down sub_response received into its parts
 */
#endif
