#include "net_util.h"


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
int readn (int fd, void *buf, uint16_t n)
{
	uint16_t n_remain = n;
	uint16_t n_read;
	char *curr = buf;
	
	while (n_remain > 0) {
		if ((n_read = read(fd, curr, n_remain)) < 0) {
			if (errno == EINTR) { //read interrupted by signal; read again
				n_read = 0;
			} else {
				perror("read");
				log_printf(ERROR, "reading from fd failed");
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
int writen (int fd, void *buf, uint16_t n)
{
	uint16_t n_remain = n;
	uint16_t n_written;
	char *curr = buf;
	
	while (n_remain > 0) {
		if ((n_written = write(fd, curr, n_remain)) <= 0) {
			if (n_written < 0 && errno == EINTR) { //write interrupted by signal, write again
				n_written = 0;
			} else {
				perror("write");
				log_printf(ERROR, "writing to fd failed");
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
uint8_t* make_buf (net_msg_t msg_type, uint16_t len_pb)
{
	uint8_t* send_buf = malloc(len_pb + 3);
	*send_buf = (uint8_t) msg_type; // Can cast since we know net_msg_t has < 10 options
	uint16_t* ptr_16 = (uint16_t*) (send_buf + 1);
	*ptr_16 = len_pb;
	// log_printf(DEBUG, "prepped buffer, len %d, ptr16 %d, msg_type %d, send buf %d %d %d", len_pb, *ptr_16, msg_type, *send_buf, *(send_buf+1), *(send_buf+2));
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
 *    - 1: successful return
 *    - 0: EOF encountered when reading from fd
 *    - -1: Error encountered when reading from fd
 */
int parse_msg (int fd, net_msg_t *msg_type, uint16_t *len_pb, uint8_t** buf)
{
	int result;
	uint8_t type;
	//read one byte -> determine message type
	if ((result = readn(fd, &type, 1)) <= 0) {
		return result;
	}
	*msg_type = type;

	//read two bytes -> determine message length
	if ((result = readn(fd, len_pb, 2)) <= 0) {
		return result;
	}
	
	*buf = malloc(*len_pb);
	//read len_pb bytes -> put into buffer
	if ((result = readn(fd, *buf, *len_pb)) <= 0) {
		free(buf);
		return result;
	}
	// log_printf(DEBUG, "parse_msg: type %d len %d buf %d", *msg_type, *len_pb, *buf);
	return 1;
}
