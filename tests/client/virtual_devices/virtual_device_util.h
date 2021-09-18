/**
 * An API of utility functions for virtual devices to communicate with dev handler
 */

#ifndef VIRTUAL_DEV_UTIL_H
#define VIRTUAL_DEV_UTIL_H

#include "../../../dev_handler/message.h"
#include "../../../runtime_util/runtime_util.h"

/**
 * Builds an ACKNOWLEDGEMENT message.
 * Arguments:
 *    type: The type of device
 *    year: The year of the device
 *    uid: The uid of the device
 * Returns:
 *    A message of type ACKNOWLEDGEMENT
 *      Payload: type, year, then uid
 *      payload_length: sizeof(type) + sizeof(year) + sizeof(uid)
 *      max_payload_length: same as above
 */
message_t* make_acknowledgement(uint8_t type, uint8_t year, uint64_t uid);

/**
 * Receives a message
 * Arguments:
 *    fd: File descriptor to read from
 *    msg: message_t to be populated with parsed message
 * Returns:
 *    0 on success
 *    1 on bad read
 *    2 on incorrect checksum
 */
int receive_message(int fd, message_t* msg);

/**
 * Sends a message
 * Arguments:
 *    fd: File descriptor to write to
 *    msg: message_t to be sent
 */
void send_message(int fd, message_t* msg);

/**
 * Processes a DEVICE_WRITE message, writing to params as appropriate
 * Arguments:
 *    type: The device of device being written to
 *    dev_write: A DEVICE_WRITE message to process
 *    params: Array of params to be written to
 */
void device_write(uint8_t type, message_t* dev_write, param_val_t params[]);

/**
 * Builds a DEVICE_DATA message, reading params as appropriate
 * Arguments:
 *    type: The type of device being read from
 *    pmap: bitmap indicating which params should be read into DEVICE_DATA
 *          i-th bit is on iff PARAMS[i] should be packaged in the DEVICE_DATA message
 *    params: Array of params to be read from
 * Returns:
 *    a DEVICE_DATA message
 */
message_t* make_device_data(uint8_t type, uint32_t pmap, param_val_t params[]);

/**
 * Executes the lowcar protocol, receiving/responding to messages, and sending
 * messages as appropriate
 * Arguments:
 *    fd: The file descriptor to read from and write to
 *    type: The device type
 *    year: The device year
 *    uid: The device uid
 *    params: Array of parameters to work with
 *    device_actions: Function pointer that accepts an array of params and
 *      modifies the param values
 *    action_interval: Number of milliseconds between each call to device_actions(). Set to -1 to disable
 */
void lowcar_protocol(int fd, uint8_t type, uint8_t year, uint64_t uid,
                     param_val_t params[], void (*device_actions)(param_val_t[]), int32_t action_interval);

#endif
