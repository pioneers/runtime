#include "Messenger.h"

//************************************* MESSENGER CLASS CONSTANTS *********************************** //

const int Messenger::MESSAGEID_BYTES = 1;		//bytes in message ID field of packet
const int Messenger::PAYLOAD_SIZE_BYTES = 1; 	//bytes in payload size field of packet
const int Messenger::CHECKSUM_BYTES = 1; 		//bytes in checksum field of packet

const int Messenger::UID_DEVICE_BYTES = 2; 		//bytes in device type field of uid
const int Messenger::UID_YEAR_BYTES = 1; 		//bytes in year field of uid
const int Messenger::UID_ID_BYTES = 8; 			//bytes in uid field of uid

//************************************** MESSENGER CLASS METHODS ************************************//

Messenger::Messenger ()
{
	Serial.begin(115200); //open Serial (USB) connection
}

//TODO: check buffer size
Status Messenger::send_message (MessageID msg_id, message_t *msg, uint32_t params, uint16_t delay, dev_id_t *uid)
{
	//build msg for heartbeat- and subscription-related messages
	if (msg_id != MessageID::LOG) {
		if (build_msg(msg_id, msg, params, delay, uid) == Status::PROCESS_ERROR)
		{
			return Status::PROCESS_ERROR;
		}
	}

	size_t msg_len = msg->payload_length + Messenger::MESSAGEID_BYTES + Messenger::PAYLOAD_SIZE_BYTES + Messenger::CHECKSUM_BYTES;
    uint8_t data[msg_len];
    uint8_t cobs_buf[2 + msg_len + 1]; //cobs encoding adds at most 2 overhead and one leading 0
	size_t cobs_len; //length of cobs-encoded data array, as reported by cobs_encode()
	uint8_t written; //number of bytes written to serial

    message_to_byte(data, msg);
    data[msg_len - Messenger::CHECKSUM_BYTES] = checksum(data, msg_len - Messenger::CHECKSUM_BYTES); //put the checksum into data

    cobs_buf[0] = 0x00; //set overhead bytes to 0 as placeholder
    cobs_len = cobs_encode(&cobs_buf[2], data, msg_len);
    cobs_buf[1] = (byte) cobs_len;

    written = Serial.write(cobs_buf, 2 + cobs_len); //write to serial port

	return (written == 2 + cobs_len) ? Status::SUCCESS : Status::PROCESS_ERROR;
}

//TODO: check buffer size
//TODO: be more specific about errors and maybe define the error types in defs.h
Status Messenger::read_message (message_t *msg)
{
	//if nothing to read
	if (!Serial.available()) {
		return Status::NO_DATA;
	}

	//find the start of packet
	int last_byte_read = -1;
	while (Serial.available()) {
		last_byte_read = Serial.read();
		if (last_byte_read == 0) {
			break;
		}
	}

	if (last_byte_read != 0) { //no start of packet found
		return Status::MALFORMED_DATA;
	}
	if (Serial.available() == 0 || Serial.peek() == 0) { //no packet length found
		return Status::MALFORMED_DATA;
	}

	uint8_t data[MAX_PAYLOAD_SIZE + Messenger::MESSAGEID_BYTES + Messenger::PAYLOAD_SIZE_BYTES];
	uint8_t cobs_buf[MAX_PAYLOAD_SIZE + Messenger::MESSAGEID_BYTES + Messenger::PAYLOAD_SIZE_BYTES + 1]; //for leading zero
	size_t cobs_len = Serial.read();
	size_t read_len = Serial.readBytesUntil(0x00, (char *)cobs_buf, cobs_len);
	if (cobs_len != read_len || cobs_decode(data, cobs_buf, cobs_len) < 3) {
		return Status::PROCESS_ERROR;
	}

	uint8_t message_id = data[0];
	uint8_t payload_length = data[1];
	uint8_t expected_chk = checksum(data, payload_length + Messenger::MESSAGEID_BYTES + Messenger::PAYLOAD_SIZE_BYTES);
	uint8_t received_chk = data[Messenger::MESSAGEID_BYTES + Messenger::PAYLOAD_SIZE_BYTES + payload_length];
	if (received_chk != expected_chk) { //if checksums don't match (no need to empty cobs buffer)
		return Status::MALFORMED_DATA;
	}
	//copy received data into msg
	msg->message_id = (MessageID) message_id;
	msg->payload_length = payload_length;
	memcpy(msg->payload, &data[Messenger::MESSAGEID_BYTES + Messenger::PAYLOAD_SIZE_BYTES], payload_length);
	return Status::SUCCESS;
}

//************************************** HELPER METHODS *****************************************//

//expects msg to exist
//builds the appropriate payload in msg according to msg_id, or doesn't do anything if msg should already be built
//returns Status to report on success/failure
Status Messenger::build_msg (MessageID msg_id, message_t *msg, uint32_t params, uint16_t delay, dev_id_t *uid)
{
	int status = 0;
	uint8_t fill_data[1] = {0};
	msg->message_id = msg_id;
	if (msg_id == MessageID::HEARTBEAT_REQUEST) {
		msg->payload[0] = this->hb_counter;
	    msg->payload_length = 1;
		printf("Sending HeartBeatRequest %d\n", (int8_t) this->hb_counter);
		this->hb_counter++;
	} else if (msg_id == MessageID::HEARTBEAT_RESPONSE) {
		msg->payload[0] = this->got_hb_req;
	    msg->payload_length = 1;
	} else if (msg_id == MessageID::SUBSCRIPTION_RESPONSE) {
	    msg->payload_length = 0;

	    status += append_payload(msg, (uint8_t *) &params, sizeof(params)); //append device param subscriptions
	    status += append_payload(msg, (uint8_t *) &delay, sizeof(delay)); //append heartbeat delay
	    status += append_payload(msg, (uint8_t *) &uid->device_type, Messenger::UID_DEVICE_BYTES); //append device type
	    status += append_payload(msg, (uint8_t *) &uid->year, Messenger::UID_YEAR_BYTES); //append year
	    status += append_payload(msg, (uint8_t *) &uid->id, Messenger::UID_ID_BYTES); //append uid
	}
	return (status < 0 || msg->payload_length > MAX_PAYLOAD_SIZE) ? Status::PROCESS_ERROR : Status::SUCCESS;
}

//appends DATA with length LENGTH to the end of the payload array of MSG
//returns -1 if payload has become too large; 0 on success
int Messenger::append_payload(message_t *msg, uint8_t *data, uint8_t length)
{
	memcpy(&(msg->payload[msg->payload_length]), data, length);
	msg->payload_length += length;
	return (msg->payload_length > MAX_PAYLOAD_SIZE) ? -1 : 0;
}

//stores members of MSG into array DATA
void Messenger::message_to_byte(uint8_t *data, message_t *msg)
{
	data[0] = (uint8_t) msg->message_id; //first byte is messageID
	data[1] = msg->payload_length; //second byte is payload length
	for (int i = 0; i < msg->payload_length; i++) { //copy the payload in one byte at a time
		data[i + Messenger::MESSAGEID_BYTES + Messenger::PAYLOAD_SIZE_BYTES] = msg->payload[i];
	}
}

//returns an 8-bit checksum of data array, computed by
//bitwise-XOR'ing each byte in order with the checksum
uint8_t Messenger::checksum (uint8_t *data, int length)
{
	uint8_t chk = data[0];
	for (int i = 1; i < length; i++) {
		chk ^= data[i];
	}
	return chk;
}

//******************************************** COBS ENCODING ********************************//
/* Cobs, short for Consistent Overhead Byte Stuffing, is an algorithm for preparing / encoding data for
 * transport. Read more here: https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
 */

#define finish_block() {		\
	*block_len_loc = block_len; \
	block_len_loc = dst++;      \
	out_len++;                  \
	block_len = 0x01;           \
}

// Encodes src into dst and returns the size of dst. Note that dst will have no
// more overhead than 1 byte per 254 bytes. src must not overlap dst.
size_t Messenger::cobs_encode (uint8_t *dst, const uint8_t *src, size_t src_len)
{
	const uint8_t *end = src + src_len;
	uint8_t *block_len_loc = dst++;
	uint8_t block_len = 0x01;
	size_t out_len = 0;

	while (src < end) {
		if (*src == 0) {
			finish_block();
		} else {
			*dst++ = *src;
			block_len++;
			out_len++;
			if (block_len == 0xFF) {
				finish_block();
			}
		}
		src++;
	}
	finish_block();

	return out_len;
}

// Decodes src into dst and returns the size of dst. src may overlap dst.
size_t Messenger::cobs_decode(uint8_t *dst, const uint8_t *src, size_t src_len)
{
	const uint8_t *end = src + src_len;
	size_t out_len = 0;

	while (src < end) {
		uint8_t code = *src++;
		for (uint8_t i = 1; i < code; i++) {
			*dst++ = *src++;
			out_len++;
			if (src > end) { // Bad packet
				return 0;
			}
		}
		if (code < 0xFF && src != end) {
			*dst++ = 0;
			out_len++;
		}
	}
	return out_len;
}

/**
 *	Used to printf from Arduino.
 *	Adds the formatted string to a queue to be sent over serial on lowcar_flush()
 *	DEV_HANDLER will process the log and send to runtime logger
 */
void Messenger::lowcar_printf(char* format, ...) {
	// Double the queue size if it's full
	if (this->num_logs == this->log_queue_max_size) {
		this->log_queue = (char**) realloc(this->log_queue, 2 * this->log_queue_max_size * MAX_PAYLOAD_SIZE);
		this->log_queue_max_size *= 2;
	}
	// Add the new formatted log to the queue
	va_list args;
    va_start(args, format);
    vsprintf(this->log_queue[this->num_logs], format, args);
    va_end(args);
	// Increment the number of logs
	this->num_logs++;
}

/**
 *	Sends strings in the log queue to DEV_HANDLER and clears the queue
 */
void Messenger::lowcar_flush() {
	message_t log;
	// For each log, send a new message
	for (int i = 0; i < this->num_logs; i++) {
		log.payload_length = strlen(this->log_queue[i]) + 1; // Null terminator character
		// Copy string into payload
		strcpy((char*) log.payload, this->log_queue[i]);
		this->send_message(MessageID::LOG, &log);
	}
	// "Clear" the queue
	this->num_logs = 0;
}
