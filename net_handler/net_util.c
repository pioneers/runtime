#include "net_util.h"

void error (char *msg)
{
	perror(msg);
	log_runtime(ERROR, msg); //send the message to the logger
}

//read n bytes from fd into buf; return number of bytes read into buf
//return -1 if error, return 0 if EOF
ssize_t readn (int fd, void *buf, size_t n)
{
	size_t n_remain = n;
	ssize_t n_read;
	char *curr = buf;
	
	while (n_remain > 0) {
		printf("n_remain = %zu\n", n_remain);
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

//write n bytes from buf to fd; return number of bytes written to fd
//return -1 on error
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

//prepares send_buf by prepending msg_type into first byte, and len_pb into second and third bytes
//caller must free send_buf
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
	*uint16_ptr = *len_pb_uint16;                //insert len_pb_uint16 into send_buf[1] and send_buf[2]
	return send_buf;
}

//reads one byte from *fd and puts it in msg_type; reads two more bytes from *connfd
//and puts it in len_pb, then reads len_pb bytes into buf
//returns -1 if EOF read (peer disconnected); returns 0 on success
int parse_msg (int *fd, net_msg_t *msg_type, uint16_t *len_pb, uint8_t *buf)
{
	ssize_t result;
	
	//read one byte -> determine message type
	if ((result = readn(*fd, &msg_type, 1)) <= 0) {
		printf("here\n");
		if (result == 0) { return -1; }
		error("read: received EOF when attempting to get msg type");
	}
	
	//read two bytes -> determine message length
	if ((result = readn(*fd, len_pb, 2)) <= 0) {
		printf("here1\n");
		if (result == 0) { return -1; }
		error("read: received EOF when attempting to get msg len");
	}
	
	printf("len_pb = %u\n", *len_pb);
	for (int i = 0; i < 16; i++) {
		printf("%d", (*len_pb >> i) & 1 ? 1 : 0);
	}
	printf("\n");
	
	//read len_pb bytes -> put into buffer
	if ((result = readn(*fd, buf, (size_t) *len_pb)) <= 0) {
		printf("here2\n");
		if (result == 0) { return -1; }
		error("read: received EOF when attempting to get msg");
	}
	
	return 0;
}

//finds max of three input numbers (used to find maxfdp1 for select)
int max (int a, int b, int c) 
{
	return (a >= b && a >= c) ? a : ((b >= a && b >= c) ? b : c);
}