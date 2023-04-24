#include "Messenger.h"

// ************************ MESSENGER CLASS CONSTANTS *********************** //

// Final encoded data sizes
const int Messenger::DELIMITER_BYTES = 1;  // Bytes of the delimiter
const int Messenger::COBS_LEN_BYTES = 1;   // Bytes indicating the size of the cobs encoded message

// Message sizes
const int Messenger::MESSAGEID_BYTES = 1;     // Bytes in message ID field of packet
const int Messenger::PAYLOAD_SIZE_BYTES = 1;  // Bytes in payload size field of packet
const int Messenger::CHECKSUM_BYTES = 1;      // Bytes in checksum field of packet

// Sizes for ACKNOWLEDGEMENT message
const int Messenger::DEV_ID_TYPE_BYTES = 1;  // Bytes in device type field of dev id
const int Messenger::DEV_ID_YEAR_BYTES = 1;  // Bytes in year field of dev id
const int Messenger::DEV_ID_UID_BYTES = 8;   // Bytes in uid field of dev id

// ************************* MESSENGER CLASS METHODS ************************ //

Messenger::Messenger(bool is_hardware_serial, HardwareSerial *hw_serial_port) {
    // Get a new GeneralSerial object to use with this device (will be Serial by default, which is typical)
    // Then, open a serial (USB) connection on that port
    this->serial_object = new GeneralSerial(is_hardware_serial, hw_serial_port);
    this->serial_object->begin(115200);
    
    // A queue initialized with room for 10 strings each of size MAX_PAYLOAD_SIZE
    this->log_queue_max_size = 10;
    this->log_queue = (char**) malloc(sizeof(char*) * this->log_queue_max_size);
    for (int i = 0; i < this->log_queue_max_size; i++) {
        this->log_queue[i] = (char*) malloc(sizeof(char) * MAX_PAYLOAD_SIZE);
    }
    this->num_logs = 0;
}

Status Messenger::send_message(MessageID msg_id, message_t* msg, dev_id_t* dev_id) {
    // Fill MessageID field
    msg->message_id = msg_id;

    /*
     * Build the message
     * All other Message Types (DEVICE_DATA, LOG) should already be built (if needed)
     */
    if (msg_id == MessageID::ACKNOWLEDGEMENT) {
        int status = 0;
        status += append_payload(msg, (uint8_t*) &dev_id->type, Messenger::DEV_ID_TYPE_BYTES);
        status += append_payload(msg, (uint8_t*) &dev_id->year, Messenger::DEV_ID_YEAR_BYTES);
        status += append_payload(msg, (uint8_t*) &dev_id->uid, Messenger::DEV_ID_UID_BYTES);

        if (status != 0) {
            return Status::PROCESS_ERROR;
        }
    }

    // Serialize the message into byte array
    size_t msg_len = Messenger::MESSAGEID_BYTES + Messenger::PAYLOAD_SIZE_BYTES + msg->payload_length + Messenger::CHECKSUM_BYTES;
    uint8_t data[msg_len];
    message_to_byte(data, msg);
    data[msg_len - Messenger::CHECKSUM_BYTES] = checksum(data, msg_len - Messenger::CHECKSUM_BYTES);  // put the checksum into data

    // Cobs encode the byte array
    uint8_t cobs_buf[Messenger::DELIMITER_BYTES + Messenger::COBS_LEN_BYTES + msg_len + 1];  // Cobs encoding adds at most 1 byte overhead
    cobs_buf[0] = 0x00;                                                                      // Start with the delimiter
    size_t cobs_len = cobs_encode(&cobs_buf[2], data, msg_len);
    cobs_buf[1] = (byte) cobs_len;

    // Write to serial
    uint8_t written = this->serial_object->write(cobs_buf, Messenger::DELIMITER_BYTES + Messenger::COBS_LEN_BYTES + cobs_len);

    // Clear the message for the next send
    msg->message_id = MessageID::NOP;
    msg->payload_length = 0;
    memset(msg->payload, 0, MAX_PAYLOAD_SIZE);

    return (written == Messenger::DELIMITER_BYTES + Messenger::COBS_LEN_BYTES + cobs_len) ? Status::SUCCESS : Status::PROCESS_ERROR;
}

Status Messenger::read_message(message_t* msg) {
    // Check if there's something to read
    if (!this->serial_object->available()) {
        return Status::NO_DATA;
    }

    // Find the start of the packet (the delimiter)
    int last_byte_read = -1;
    while (this->serial_object->available()) {
        last_byte_read = this->serial_object->read();
        if (last_byte_read == 0) {  // Byte 0x00 is the delimiter
            break;
        }
    }

    if (last_byte_read != 0) {  // no start of packet found
        return Status::MALFORMED_DATA;
    }
    if (this->serial_object->available() == 0 || this->serial_object->peek() == 0) {  // no packet length found
        return Status::MALFORMED_DATA;
    }

    // Read the cobs len (how many bytes left in the message)
    size_t cobs_len = this->serial_object->read();

    // Read the rest of the message into buffer
    uint8_t cobs_buf[cobs_len];
    size_t read_len = this->serial_object->readBytesUntil(0x00, (char*) cobs_buf, cobs_len);  // Read cobs_len bytes or until the next delimiter (whichever is first)
    if (cobs_len != read_len) {
        return Status::PROCESS_ERROR;
    }

    // Decode the cobs-encoded message into a buffer
    uint8_t data[cobs_len];  // The decoded message will be smaller than COBS_LEN
    cobs_decode(data, cobs_buf, cobs_len);

    uint8_t message_id = data[0];
    uint8_t payload_length = data[1];

    // Verify that the received message has the correct checksum
    uint8_t expected_chk = checksum(data, Messenger::MESSAGEID_BYTES + Messenger::PAYLOAD_SIZE_BYTES + payload_length);
    uint8_t received_chk = data[Messenger::MESSAGEID_BYTES + Messenger::PAYLOAD_SIZE_BYTES + payload_length];
    if (received_chk != expected_chk) {
        return Status::MALFORMED_DATA;
    }

    // Populate MSG with received data
    msg->message_id = (MessageID) message_id;
    msg->payload_length = payload_length;
    memcpy(msg->payload, &data[Messenger::MESSAGEID_BYTES + Messenger::PAYLOAD_SIZE_BYTES], payload_length);
    return Status::SUCCESS;
}

void Messenger::lowcar_printf(char* format, ...) {
    // Double the queue size if it's full
    if (this->num_logs == this->log_queue_max_size) {
        this->log_queue = (char**) realloc(this->log_queue, sizeof(char*) * 2 * this->log_queue_max_size);
        for (int i = this->log_queue_max_size; i < 2 * this->log_queue_max_size; i++) {
            this->log_queue[i] = (char*) malloc(sizeof(char) * MAX_PAYLOAD_SIZE);
        }
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

void Messenger::lowcar_print_float(char* name, float val) {
    char sign = (val < 0.0) ? '-' : ' ';  // Holds sign of the value
    if (val < 0.0) {
        val *= -1.0;
    }
    int whole_val = (int) val;                           // Part of val to the left of the decimal point
    int thousandths = (int) ((val - whole_val) * 1000);  // The 3 digits to the right of the decimal point

    if (sign == '-') {
        this->lowcar_printf("%s: %c%d.%03d", name, sign, whole_val, thousandths);
    } else {
        this->lowcar_printf("%s:%c%d.%03d", name, sign, whole_val, thousandths);
    }
}

void Messenger::lowcar_flush() {
    // don't send anything if no logs
    if (this->num_logs == 0) {
        return;
    }

    message_t log;
    // For each log, send a new message
    for (int i = 0; i < this->num_logs; i++) {
        log.payload_length = strlen(this->log_queue[i]) + 1;  // Null terminator character

        // Copy string into payload
        memcpy((char*) log.payload, this->log_queue[i], (size_t) log.payload_length);
        this->send_message(MessageID::LOG, &log);
    }
    // "Clear" the queue
    this->num_logs = 0;
}

// ***************************** HELPER METHODS ***************************** //

int Messenger::append_payload(message_t* msg, uint8_t* data, uint8_t length) {
    if (msg->payload_length + length > MAX_PAYLOAD_SIZE) {
        return -1;
    }
    memcpy(&(msg->payload[msg->payload_length]), data, length);
    msg->payload_length += length;
    return 0;
}

void Messenger::message_to_byte(uint8_t* data, message_t* msg) {
    data[0] = (uint8_t) msg->message_id;
    data[1] = msg->payload_length;
    memcpy(&data[2], msg->payload, msg->payload_length);
}

uint8_t Messenger::checksum(uint8_t* data, int length) {
    uint8_t chk = data[0];
    for (int i = 1; i < length; i++) {
        chk ^= data[i];
    }
    return chk;
}

// ***************************** COBS ENCODING ****************************** //

#define finish_block()              \
    {                               \
        *block_len_loc = block_len; \
        block_len_loc = dst++;      \
        out_len++;                  \
        block_len = 0x01;           \
    }

size_t Messenger::cobs_encode(uint8_t* dst, const uint8_t* src, size_t src_len) {
    const uint8_t* end = src + src_len;
    uint8_t* block_len_loc = dst++;
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

size_t Messenger::cobs_decode(uint8_t* dst, const uint8_t* src, size_t src_len) {
    const uint8_t* end = src + src_len;
    size_t out_len = 0;

    while (src < end) {
        uint8_t code = *src++;
        for (uint8_t i = 1; i < code; i++) {
            *dst++ = *src++;
            out_len++;
            if (src > end) {  // Bad packet
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
