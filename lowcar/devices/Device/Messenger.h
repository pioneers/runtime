/**
 * A class used by each device to send and receive messages to and from the device handler
 */

#ifndef MESSENGER_H
#define MESSENGER_H

#include "defs.h"
#include <stdarg.h> // va_start, va_list, va_arg, va_end

class Messenger {
public:
	/* constructor; opens Serial connection */
	Messenger ();

	/* Handles any type of message and fills in appropriate parameters, then sends onto Serial port
	 * msg_id: The MessageID to populate msg->message_id
	 * msg: The message to send
	 * dev_id: The id of the device. Used only for ACKNOWLEDGEMENT messages
	 *
	 * Return a Status enum to report on success/failure
	 * Resets msg->message_id and msg->payload_length
	 */
	Status send_message (MessageID msg_id, message_t *msg, dev_id_t *dev_id = NULL);

	/* Reads in data from serial port and puts results into msg
	 * Return a Status enum to report on success/failure
	 */
	Status read_message (message_t *msg);

	// Logging
	void lowcar_printf(char* format, ...);            // Queues a log. Same usage as printf()
	void lowcar_print_float(char *name, float val);   // Workaround to print a float value
	void lowcar_flush();                              // Sends queued logs over serial

private:
	//protocol constants
	const static int DELIMITER_BYTES;
	const static int COBS_LEN_BYTES;

	const static int MESSAGEID_BYTES;    //bytes in message ID field of packet
	const static int PAYLOAD_SIZE_BYTES; //bytes in payload size field of packet
	const static int CHECKSUM_BYTES;     //bytes in checksum field of packet

	const static int DEV_ID_TYPE_BYTES;  //bytes in device type field of dev_id
	const static int DEV_ID_YEAR_BYTES;  //bytes in year field of dev_id
	const static int DEV_ID_UID_BYTES;   //bytes in uid field of dev_id

	//private variables
	uint8_t log_queue_max_size;
	char **log_queue;
	uint8_t num_logs;

	//helper methods; see source file for more detailed description of functionality
	int append_payload (message_t *msg, uint8_t *data, uint8_t length);
	void message_to_byte (uint8_t *data, message_t *msg);
	uint8_t checksum (uint8_t *data, int length);
	size_t cobs_encode (uint8_t *dst, const uint8_t *src, size_t src_len);
	size_t cobs_decode(uint8_t *dst, const uint8_t *src, size_t src_len);
};

#endif
