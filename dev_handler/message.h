/**
 * Serves as a utility API for DEV_HANDLER
 * Functions to build, encode, decode, serialize, and parse messages
 * Each message, when serialized is in the following format:
 * [delimiter][Length of cobs encoded message][Cobs encoded message]
 * The cobs encoded message, when decoded is in the following format:
 * [message id][payload length][payload][checksum]
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include "../logger/logger.h"
#include "../runtime_util/runtime_util.h"

/* The maximum number of milliseconds to wait between each DEVICE_PING from a device
 * Waiting for this long will exit all threads for that device (doing cleanup as necessary)
 * It's reasonable to match the TIMEOUT that the Arduino uses.
 */
#define TIMEOUT 1000

// The number of milliseconds between each DEVICE_PING sent to the device
#define PING_FREQ 250

// The size in bytes of the message delimiter
#define DELIMITER_SIZE 1
// The size in bytes of the section specifying the length of the cobs encoded message
#define COBS_LENGTH_SIZE 1
// The size in bytes of the section specifying the type of message being encoded
#define MESSAGE_ID_SIZE 1
// The size in bytes of the section specifying the length of the payload in the message
#define PAYLOAD_LENGTH_SIZE 1
// The size in bytes of the param bitmap in DEVICE_WRITE and DEVICE_DATA payloads
#define BITMAP_SIZE (MAX_PARAMS / 8)
// The size in bytes of the section specifying the device id for ACKNOWLEDGEMENT
#define DEVICE_ID_SIZE 10
// The size in bytes of the section specifying the checksum of the message id, the payload length, and the payload itself
#define CHECKSUM_SIZE 1
// The length of the largest payload in bytes, which may be reached for DEVICE_WRITE and DEVICE_DATA message types.
#define MAX_PAYLOAD_SIZE (BITMAP_SIZE + (MAX_PARAMS * sizeof(float)))  // Bitmap + Each param (may be floats)

// The types of messages
typedef enum {
    NOP = 0x00,              // Dummy message
    DEVICE_PING = 0x01,      // To lowcar
    ACKNOWLEDGEMENT = 0x02,  // To dev handler
    DEVICE_WRITE = 0x03,     // To lowcar
    DEVICE_DATA = 0x04,      // To dev handler
    LOG = 0x05               // To dev handler
} message_id_t;

// A struct defining a message to be sent over serial
typedef struct {
    message_id_t message_id;
    uint8_t* payload;           // Array of bytes
    size_t payload_length;      // The current number of bytes in payload
    size_t max_payload_length;  // The maximum length of the payload for the specific message_id
} message_t;

// ******************************** Utility ********************************* //

/**
 * Prints a byte array byte by byte in hex
 * Arguments:
 *    data: Byte array to be printed
 *    len: The length of the data array
 */
void print_bytes(uint8_t* data, size_t len);

// ************************* MESSAGE CONSTRUCTORS *************************** //
// Messages built from these constructors MUST be decalloated with destroy_message()

/**
 * Utility function to allocate memory for an empty message
 * To be used with parse_message()
 * Arguments:
 *    payload_size: The size of memory to allocate for the payload
 * Returns:
 *    A message of type NOP, payload_length 0, and max_payload_length PAYLOAD_SIZE
 */
message_t* make_empty(ssize_t payload_size);

/**
 * Builds a DEVICE_PING message
 * Returns:
 *    A message of type DEVICE_PING
 *      payload_length 0
 *      max_payload_length 0
 */
message_t* make_ping();

/**
 * Builds a DEVICE_WRITE
 * Arguments:
 *    dev_type: The type of device. Used to verify params are writeable
 *    pmap: param bitmap indicating which parameters will be written to
 *    param_values: An array of the parameter values.
 *      The i-th bit in PMAP is on if and only if its value is in the i-th index of PARAM_VALUES
 * Returns:
 *    A message of type DEVICE_WRITE
 *      Payload: pmap followed by, each of the param_values specified
 *      payload_length: sizeof(pmap) + sizeof(all the values in PARAM_VALUES)
 *      max_payload_length: same as above
 */
message_t* make_device_write(uint8_t dev_type, uint32_t pmap, param_val_t param_values[]);

/**
 * Frees the memory allocated for the message struct and its payload.
 * Arguments:
 *    message: The message to have its memory deallocated.
 */
void destroy_message(message_t* message);

// ********************* SERIALIZE AND PARSE MESSAGES *********************** //

/**
 * Calculates the largest length possible of a cobs-encoded msg
 * Used when allocating a buffer to parse a message into via parse_message()
 * Arguments:
 *    msg: The message to be serialized to a byte array
 * Returns:
 *    The size of a byte array that should be allocated to serialize the message into
 */
size_t calc_max_cobs_msg_length(message_t* msg);

/**
 * Serializes then cobs encodes a message into a byte array
 * Arguments:
 *    msg: the message to serialize
 *    cobs_encoded: empty buffer to be filled with the cobs-encoded message
 *    len: the length of COBS_ENCODED. Should be at least calc_max_cobs_msg_length(msg)
 * Returns:
 *    The size of COBS_ENCODED that was actually populated
 *    -1 if len is too small (less than calc_max_cobs_msg_length)
 */
ssize_t message_to_bytes(message_t* msg, uint8_t cobs_encoded[], size_t len);

/**
 * Cobs decodes a byte array and populates the fields of input message
 * Arguments:
 *    data: A byte array containing a cobs encoded message.
 *      data[0] should be the delimiter. data[1] should be cobs_len
 *    empty_msg: A message to be populated.
 *      Payload must be properly allocated memory. Use make_empty()
 * Returns:
 *    0 if successful parsing
 *    1 if incorrect checksum
 *    2 if max_payload_length is too small
 *    3 if invalid message (decoded message has invalid length)
 */
int parse_message(uint8_t data[], message_t* empty_msg);

/**
 * Reads the parameter values from a DEVICE_DATA message into param_val_t[]
 * Arguments:
 *    dev_type: The type of the device that the message was sent from
 *    dev_data: The DEVICE_DATA message to unpack
 *    vals: An array of param_val_t structs to be populated with the values from the message.
 * NOTE: The length of vals MUST be at LEAST the number of params sent in the DEVICE_DATA message
 * Allocate MAX_PARAMS param_val_t structs to guarantee this
 */
void parse_device_data(uint8_t dev_type, message_t* dev_data, param_val_t vals[]);

#endif
