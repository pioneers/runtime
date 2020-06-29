#include "net_util.h"

/*
 * General error wrapper for net_handler. Sends provided msg to logger.
 * Arguments:
 *    - char *msg: pointer to message to be associated with the error
 */
void error (char *msg)
{
	perror(msg);
	log_runtime(ERROR, msg); //send the message to the logger
}

/*
 * Read n bytes from fd into buf; return number of bytes read into buf (deals with interrupts and unbuffered reads)
 * Arguments:
 *    - int fd: file descriptor to read from
 *    - void *buf: pointer to location to copy read bytes into
 *    - size_t n: number of bytes to read
 * Return:
 *    - > 0: number of bytes read into buf
 *    - 0: read EOF on fd
 *    - -1: read errored out
 */
ssize_t readn (int fd, void *buf, size_t n)
{
	size_t n_remain = n;
	ssize_t n_read;
	char *curr = buf;
	
	while (n_remain > 0) {
		if ((n_read = read(fd, curr, n_remain)) < 0) {
			if (errno == EINTR) { //read interrupted by signal; read again
				n_read = 0;
			} else {
				error("read: reading from connfd failed @shep");
				return -1;
			}
		} else if (n_read == 0) { //received EOF
			return 0;
		}
		n_remain -= n_read;
		curr += n_read;
	}
	return (n - n_remain);
}

/*
 * Read n bytes from buf to fd; return number of bytes written to buf (deals with interrupts and unbuffered writes)
 * Arguments:
 *    - int fd: file descriptor to write to
 *    - void *buf: pointer to location to read from
 *    - size_t n: number of bytes to write
 * Return:
 *    - >= 0: number of bytes written into buf
 *    - -1: write errored out
 */
ssize_t writen (int fd, const void *buf, size_t n)
{
	size_t n_remain = n;
	ssize_t n_written;
	const char *curr = buf;
	
	while (n_remain > 0) {
		if ((n_written = write(fd, curr, n_remain)) <= 0) {
			if (n_written < 0 && errno == EINTR) { //write interrupted by signal, write again
				n_written = 0;
			} else {
				error("write: writing to connfd failed @shep");
				return -1;
			}
		}
		n_remain -= n_written;
		curr += n_written;
	}
	return n;
}

/*
 * Prepares a buffer of uint8_t for receiving a packed protobuf message of the specified type and length.
 * Also converts the specified length of message from unsigned to uint16_t and returns it in the third argument.
 * Returns the prepared buffer containing the message type in the first element and the length in the second and third elements.
 * The prepared buffer must be freed by the caller.
 * Arguments:
 *    - net_msg_t msg_type: one of the message types defined in net_util.h
 *    - unsigned len_pb: length of the serialized bytes returned by the protobuf function *__get_packed_size() 
 *    - uint16_t *len_pb_uint16: upon successful return, will hold the given len_pb as a uint16_t (useful for pointer arithmetic)
 * Return:
 *    - pointer to uint8_t that was malloc'ed, with the first three bytes set appropriately and with exactly enough space
 *      to fit the rest of the serialized message into
 */
uint8_t *prep_buf (net_msg_t msg_type, unsigned len_pb, uint16_t *len_pb_uint16)
{
	char tmp[4];                 //used for converting msg_type and len_pb to uint8_t and uint16_t, respectively
	uint8_t *send_buf;           //buffer that will eventually be used to send message
	uint8_t msg_type_uint8;      //converted value for msg_type
	uint16_t *uint16_ptr;        //used to insert a uint16_t into an array of uint8_t
	
	//convert values
	sprintf(tmp, "%u", msg_type);
	msg_type_uint8 = (uint8_t) strtoul((const char *) tmp, NULL, 0);
 	sprintf(tmp, "%u", len_pb);
	*len_pb_uint16 = (uint16_t) strtoul((const char *) tmp, NULL, 0);
	
	send_buf = (uint8_t *) malloc(sizeof(uint8_t) * (*len_pb_uint16 + 3)); // +3 because 1 byte for message type, 2 bytes for message length
	*send_buf = msg_type_uint8;                 //write the message type to the first byte of send_buf    
	uint16_ptr = (uint16_t *)(send_buf + 1);    //make uint16_ptr point to send_buf[1], where we will insert len_pb_uint16
	*uint16_ptr = *len_pb_uint16;               //insert len_pb_uint16 into send_buf[1] and send_buf[2]
	return send_buf;
}

/*
 * Parses a message from the given file descriptor into its separate components and stores them in provided pointers
 * Arguments:
 *    - int *fd: pointer to file descriptor from which to read the incoming message
 *    - net_msg_t *msg_type: message type of the incoming message will be stored in this location upon successful return
 *    - uint16_t *len_pb: serialized length, in bytes, of the incoming message will be stored in this location upon successful return
 *    - uint8_t *buf: serialized message will be stored starting at this location upon successful return
 * Return:
 *    - 0: successful return
 *    - -1: EOF encountered when reading from fd
 */
int parse_msg (int *fd, net_msg_t *msg_type, uint16_t *len_pb, uint8_t *buf)
{
	ssize_t result;
	
	//read one byte -> determine message type
	if ((result = readn(*fd, buf, 1)) <= 0) {
		if (result == 0) { return -1; }
		error("read: received EOF when attempting to get msg type");
	}
	*msg_type = buf[0];

	//read two bytes -> determine message length
	if ((result = readn(*fd, len_pb, 2)) <= 0) {
		if (result == 0) { return -1; }
		error("read: received EOF when attempting to get msg len");
	}
	
	//read len_pb bytes -> put into buffer
	if ((result = readn(*fd, buf, (size_t) *len_pb)) <= 0) {
		if (result == 0) { return -1; }
		error("read: received EOF when attempting to get msg");
	}
	
	return 0;
}
