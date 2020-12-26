#include "net_util.h"

uint8_t* make_buf(net_msg_t msg_type, uint16_t len_pb) {
    uint8_t* send_buf = malloc(len_pb + 3);
    if (send_buf == NULL) {
        log_printf(FATAL, "make_buf: Failed to malloc");
        exit(1);
    }
    *send_buf = (uint8_t) msg_type;  // Can cast since we know net_msg_t has < 10 options
    uint16_t* ptr_16 = (uint16_t*) (send_buf + 1);
    *ptr_16 = len_pb;
    // log_printf(DEBUG, "prepped buffer, len %d, ptr16 %d, msg_type %d, send buf %d %d %d", len_pb, *ptr_16, msg_type, *send_buf, *(send_buf+1), *(send_buf+2));
    return send_buf;
}

int parse_msg(int fd, net_msg_t* msg_type, uint16_t* len_pb, uint8_t** buf) {
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
    if ((result = readn(fd, *buf, *len_pb)) < 0) {
        free(*buf);
        return result;
    }
    // log_printf(DEBUG, "parse_msg: type %d len %d buf %d", *msg_type, *len_pb, *buf);
    return 1;
}
