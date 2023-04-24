/**
 * A class used by each device to send and receive messages to and from the device handler
 */

#ifndef MESSENGER_H
#define MESSENGER_H

#include <stdarg.h>  // va_start, va_list, va_arg, va_end
#include "defs.h"
#include "GeneralSerial.h"

class Messenger {
  public:
    /* 
     * Constructor; opens Serial connection on specified port
     * Arguments:
     *     is_hardware_serial: False by default (use Serial); set to True for devices that use SerialX pins
	 *     hw_serial_prt: Unused (NULL) by default; when is_hardware_serial == True, specify which SerialX port to use
     */
    Messenger(bool is_hardware_serial, HardwareSerial *hw_serial_port);

    /**
     * Handles any type of message and fills in appropriate parameters, then sends onto Serial port
     * Clears all of MSG's fields before returning.
     * Arguments:
     *    msg_id: The MessageID to populate msg->message_id
     *    msg: The message to send
     *    dev_id: The id of the device. Used for only ACKNOWLEDGEMENT messages
     * Returns:
     *    a Status enum to report on success/failure
     */
    Status send_message(MessageID msg_id, message_t* msg, dev_id_t* dev_id = NULL);

    /**
     * Reads in data from serial port and puts results into msg
     * Arguments:
     *    msg: A message to be populated based on read serial data.
     * Returns:
     *    a Status enum to report on success/failure
     */
    Status read_message(message_t* msg);

    // ****************************** LOGGING ******************************* //

    /**
     * Queues a LOG message to be sent to dev handler (for runtime logger)
     * Arguments:
     *    format: The string format to be sent (same usage as printf())
     */
    void lowcar_printf(char* format, ...);

    /**
     * Workaround to print a float value.
     * Use this function to print the value of a float instead of %f in lowcar_printf()
     * This is needed because floats appear as question marks in the logger
     * Sends a log with the string: "<name>: <val>"
     *  Where val is displayed up to the thousandths place
     * Arguments:
     *    name: A string describing the value of the float (ex: the variable name)
     *    val: The float to be printed alongside NAME
     */
    void lowcar_print_float(char* name, float val);

    /**
     * Sends all currently queued logs to dev handler
     * The log queue is then cleared.
     */
    void lowcar_flush();

  private:
    // protocol constants
    const static int DELIMITER_BYTES;  // The size of a packet delimiter
    const static int COBS_LEN_BYTES;   // The size the cobs length in a packet

    const static int MESSAGEID_BYTES;     // bytes in message ID field of packet
    const static int PAYLOAD_SIZE_BYTES;  // bytes in payload size field of packet
    const static int CHECKSUM_BYTES;      // bytes in checksum field of packet

    const static int DEV_ID_TYPE_BYTES;  // bytes in device type field of dev_id
    const static int DEV_ID_YEAR_BYTES;  // bytes in year field of dev_id
    const static int DEV_ID_UID_BYTES;   // bytes in uid field of dev_id

    // private variables
    uint8_t log_queue_max_size;  // The size of the log queue in bytes
    char** log_queue;            // The log queue
    uint8_t num_logs;            // The number of logs in the log queue
    GeneralSerial *serial_object;         // The Serial port to use (either Serial or Serial1)

    // *************************** HELPER METHODS *************************** //

    /**
     * Appends data to the payload of a message
     * The payload length field of the message is incremented accordingly.
     * Arguments:
     *    msg: The message whose payload is to be appended to
     *    data: The buffer containing data to append to msg->payload
     *    length: The size of DATA
     * Returns:
     *    0 on success, or
     *    -1 if LENGTH is too large (DATA can't fit in the allocated payload)
     */
    int append_payload(message_t* msg, uint8_t* data, uint8_t length);

    /**
     * Serializes a message into a byte packet to be sent over serial.
     * Arguments:
     *    data: The buffer to write the serialized message into
     *    msg: A populated message to be serialized.
     */
    void message_to_byte(uint8_t* data, message_t* msg);

    /**
     * Computes the checksum of a data buffer.
     * The checksum is the bitwise XOR of each byte in the buffer.
     * Arguments:
     *    data: The data buffer whose checksum is to be computed.
     *    length: the size of DATA
     * Returns:
     *    the checksum
     */
    uint8_t checksum(uint8_t* data, int length);

    // **************************** COBS ENCODING *************************** //

    /**
     * Cobs encodes a byte array into a buffer
     * Arguments:
     *    src: The byte array to be encoded
     *    dst: The buffer to write the encoded data into
     *    src_len: The size of SRC
     * Returns:
     *    The size of the encoded data, DST
     */
    size_t cobs_encode(uint8_t* dst, const uint8_t* src, size_t src_len);

    /**
     * Cobs decodes a byte array into a buffer
     * Arguments:
     *    src: The byte array to be decoded
     *    dst: The buffer to write the decoded data into
     *    src_len: The size of SRC
     * Returns:
     *    The size of the decoded data, DST
     */
    size_t cobs_decode(uint8_t* dst, const uint8_t* src, size_t src_len);
};

#endif
