#include "Device.h"

// Device constructor
Device::Device(DeviceType dev_id, uint8_t dev_year, bool is_hardware_serial, HardwareSerial *hw_serial_port, uint32_t timeout) {
    this->dev_id.type = dev_id;
    this->dev_id.year = dev_year;
    this->dev_id.uid = 0;  // sets a temporary value

    this->timeout = timeout;  // timeout in ms how long we tolerate not receiving a DEVICE_PING from dev handler
    this->enabled = FALSE;    // Whether or not the device currently has a connection with Runtime

	// make a new Messenger on the specified serial port
    this->msngr = new Messenger(is_hardware_serial, hw_serial_port);
    this->led = new StatusLED();

    this->last_sent_data_time = this->last_received_ping_time = this->curr_time = millis();
}

void Device::set_uid(uint64_t uid) {
    this->dev_id.uid = uid;
}

void Device::loop() {
    Status sts;
    this->curr_time = millis();
    sts = this->msngr->read_message(&(this->curr_msg));  // try to read a new message

    if (sts == Status::SUCCESS) {  // we have a message!
        switch (this->curr_msg.message_id) {
            case MessageID::DEVICE_PING:
                this->last_received_ping_time = this->curr_time;
                // If this is the first DEVICE_PING received, send an ACKNOWLEDGEMENT
                if (!this->enabled) {
                    this->msngr->send_message(MessageID::ACKNOWLEDGEMENT, &(this->curr_msg), &(this->dev_id));
                    this->msngr->lowcar_printf("Device type %d, UID 0x...%X sent ACK", (uint8_t) this->dev_id.type, this->dev_id.uid);
                    this->enabled = TRUE;
                    device_enable();
                }
                break;

            case MessageID::DEVICE_WRITE:
                device_write_params(&(this->curr_msg));
                break;

            // Receiving some other Message
            default:
                this->msngr->lowcar_printf("Unrecognized message received by lowcar device");
                break;
        }
    } else if (sts != Status::NO_DATA) {
        this->msngr->lowcar_printf("Error when reading message by lowcar device");
    }
    device_actions();  //[MOVED TO HERE]
    // If it's been too long since we received a DEVICE_PING, disable the device
    if (this->enabled && (this->timeout > 0) && (this->curr_time - this->last_received_ping_time >= this->timeout)) {
        device_reset();
        this->enabled = FALSE;
    }

    // If we still haven't gotten our first DEVICE_PING yet (or dev handler timed out), keep waiting for a DEVICE_PING
    if (!(this->enabled)) {
        return;
    }

    // do device-specific actions. This may change params
    // device_actions(); //[MOVED]

    /* Send another DEVICE_DATA with all readable parameters if DATA_INTERVAL_MS
     * milliseconds passed since the last time we sent a DEVICE_DATA
     * Note that it is possible that no parameters are readable.
     * We send an "empty" DEVICE_DATA anyways to let dev handler know we're still online
     */
    if (this->curr_time - this->last_sent_data_time >= DATA_INTERVAL_MS) {
        this->last_sent_data_time = this->curr_time;
        device_read_params(&(this->curr_msg));
        this->msngr->send_message(MessageID::DEVICE_DATA, &(this->curr_msg));
    }

    // Send any queued logs
    this->msngr->lowcar_flush();
}

// ******************** DEFAULT DEVICE-SPECIFIC METHODS ********************* //

size_t Device::device_read(uint8_t param, uint8_t* data_buf) {
    return 0;  // by default, we read 0 bytes into buffer
}

size_t Device::device_write(uint8_t param, uint8_t* data_buf) {
    return 0;  // by default, we wrote 0 bytes successfully to device
}

void Device::device_enable() {
    return;  // by default, enabling the device does nothing
}

void Device::device_reset() {
    return;  // by default, resetting the device does nothing
}

void Device::device_actions() {
    return;  // by default, device does nothing on every loop
}

// ***************************** HELPER METHODS ***************************** //

void Device::device_read_params(message_t* msg) {
    // Clear the message before building device data
    msg->message_id = MessageID::DEVICE_DATA;
    msg->payload_length = 0;
    memset(msg->payload, 0, MAX_PAYLOAD_SIZE);
    uint32_t param_bitmap = 0;

    // Loop through every parameter and attempt to read it into the buffer
    // If the parameter is readable, then turn on the bit in the param_bitmap
    msg->payload_length = PARAM_BITMAP_BYTES;
    for (uint8_t param_num = 0; param_num < MAX_PARAMS; param_num++) {
        size_t param_size = device_read(param_num, msg->payload + msg->payload_length);
        msg->payload_length += param_size;

        // If the parameter is readable
        if (param_size > 0) {
            param_bitmap |= 1 << param_num;
        }
    }

    // The first 32 bits of the payload should be set to the param_bitmap we determined
    uint32_t* payload_ptr_uint32 = (uint32_t*) msg->payload;
    *payload_ptr_uint32 = param_bitmap;
}

void Device::device_write_params(message_t* msg) {
    if (msg->message_id != MessageID::DEVICE_WRITE) {
        return;
    }

    // Param bitmap of parameters to write is at the beginning of the payload
    uint32_t param_bitmap = *((uint32_t*) msg->payload);

    // Loop over param_bitmap and attempt to write data for requested bits
    uint8_t* payload_ptr = msg->payload + PARAM_BITMAP_BYTES;
    for (uint32_t param_num = 0; (param_bitmap >> param_num) > 0; param_num++) {
        if (param_bitmap & (1 << param_num)) {
            payload_ptr += device_write((uint8_t) param_num, payload_ptr);
        }
    }
}
