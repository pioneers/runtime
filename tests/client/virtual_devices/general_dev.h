/**
 * A virtual device that behaves like a proper lowcar device
 */

#ifndef GEN_DEV_H
#define GEN_DEV_H

#include <stdlib.h> // for atoi()
#include <stdio.h>  // for print

#include "../../../dev_handler/message.h"
#include "../../../runtime_util/runtime_util.h"

#define PING_FREQ 1000

#define TIMEOUT 2500

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
message_t *make_acknowledgement(uint8_t type, uint8_t year, uint64_t uid);

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
int receive_message(int fd, message_t *msg);

/**
 * Sends a message
 * Arguments:
 *    fd: File descriptor to write to
 *    msg: message_t to be sent
 */
void send_message(int fd, message_t *msg);

/**
 * Processes a DEVICE_WRITE message, writing to params as appropriate
 * Arguments:
 *    dev_write: A DEVICE_WRITE message to process
 *    params: Array of params to be written to
 */
void device_write(message_t *dev_write, param_val_t params[]);

/**
 * Builds a DEVICE_DATA message, reading params as appropriate
 * Arguments:
 *    dev_data: message_t to be populated with the requested data
 *    pmap: bitmap indicating which params should be read into DEV_DATA
 *    params: Array of params to be read from
 */
message_t *make_device_data(uint32_t pmap, param_val_t params[]);

#endif
