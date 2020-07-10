#include "Device.h"

const float Device::MAX_SUB_INTERVAL_MS = 250.0;    // maximum tolerable subscription delay, in ms
const float Device::MIN_SUB_INTERVAL_MS = 40.0;     // minimum tolerable subscription delay, in ms

//Device constructor
Device::Device (DeviceType dev_id, uint8_t dev_year, uint32_t timeout, uint32_t ping_interval)
{
    this->dev_id.type = dev_id;
    this->dev_id.year = dev_year;
    this->dev_id.uid = 0x0123456789ABCDEF;

    this->params = 0;         // nothing subscribed to right now
    this->sub_interval = 0;   // 0 acts as flag indicating no subscription
    this->timeout = timeout;
    this->ping_interval = ping_interval;
    this->last_sub_time = this->last_sent_ping_time = this->last_received_ping_time = this->curr_time = millis();
    this->acknowledged = false;

    this->msngr = new Messenger();
    this->led = new StatusLED();

    device_enable(); //call device's enable function
}

//universal loop function
//TODO: report errors or do something with them
//TODO: do something when device disables because of time out
void Device::loop ()
{
    Status sts;
    this->curr_time = millis();
    sts = this->msngr->read_message(&(this->curr_msg)); //try to read a new message

    if (sts == Status::SUCCESS) { //we have a message!
        switch (this->curr_msg.message_id) {
            case MessageID::PING:
                this->last_received_ping_time = this->curr_time;
                this->led->quick_blink(4);
                // If this is the first PING received, send an ACKNOWLEDGEMENT
                if (!this->acknowledged) {
                    this->led->quick_blink(5);
                    if (this->msngr->send_message(MessageID::ACKNOWLEDGEMENT, &(this->curr_msg), this->params, this->sub_interval, &(this->dev_id)) == Status::PROCESS_ERROR){
                        this->led->quick_blink(5);
                    }
                    this->acknowledged = true;
                }
                break;

            case MessageID::SUBSCRIPTION_REQUEST:
                this->params = *((uint32_t *) &(this->curr_msg.payload[0]));    // Update subscribed params
                // TODO: Verify that subscribed params are readable
                this->sub_interval = *((uint16_t *) &(this->curr_msg.payload[4]));  // Update the interval at which we send DEVICE_DATA
                // Make sure sub_interval is within bounds
                this->sub_interval = min(this->sub_interval, MAX_SUB_INTERVAL_MS);
                this->sub_interval = max(this->sub_interval, MIN_SUB_INTERVAL_MS);
                break;

            case MessageID::DEVICE_WRITE:
                device_rw_all(&(this->curr_msg), RWMode::WRITE);
                break;

            // Receiving some other Message
            //default:
                //this->led->toggle();
                // break;
        }
    } else {
        // No message received
    }

    /* Send another DEVICE_DATA with subscribed parameters if this->sub_interval
     * passed since the last time we sent a DEVICE_DATA
     * If this->sub_interval == 0, don't send DEVICE_DATA */
    if ((this->sub_interval > 0) && (this->curr_time - this->last_sub_time >= this->sub_interval)) {
        this->last_sub_time = this->curr_time;
        device_rw_all(&(this->curr_msg), RWMode::READ);
        this->msngr->send_message(MessageID::DEVICE_DATA, &(this->curr_msg));
    }

    // Send another PING if this->ping_interval passed since the last time we sent a PING
    // Don't start sending PING messages until after we sent an ACKNOWLEDGEMENT
    if ((this->acknowledged) && (this->ping_interval > 0) && (this->curr_time - this->last_sent_ping_time >= this->ping_interval)) {
        this->last_sent_ping_time = this->curr_time;
        this->msngr->send_message(MessageID::PING, &(this->curr_msg));
    }

    // Send any queued logs
    if (this->msngr->num_logs != 0) {
        this->msngr->lowcar_flush();
    }

    // If it's been too long since we received a PING, disable the device
    if ((this->timeout > 0)  && (this->curr_time - this->last_received_ping_time >= this->timeout)) {
        device_disable();
    }

    device_actions(); //do device-specific actions
}

//************************************************ DEFAULT DEVICE-SPECIFIC METHODS ******************************************* //

//a device uses this function to return data about its state
uint8_t Device::device_read (uint8_t param, uint8_t *data_buf, size_t data_buf_len)
{
  return 0; //by default, we read 0 bytes into buffer
}

//a device uses this function to change a state
uint32_t Device::device_write (uint8_t param, uint8_t *data_buf)
{
  return 0; //by default, we wrote 0 bytes successfully to device
}

//a device uses this function to enable itself
void Device::device_enable ()
{
  return; //by default, enabling the device does nothing
}

//a device uses this function to disable itself
void Device::device_disable ()
{
  return; //by default, disabling the device does nothing
}

//a device uses this function to perform any continuous updates or actions
void Device::device_actions ()
{
  return; //by default, device does nothing on every loop
}

//*************************************************** HELPER METHODS ***************************************************//

/**
 *  Helper function for building outgoing DEVICE_DATA or processing received DEVICE_WRITE
 *  msg:    DEVICE_DATA to be built OR DEVICE_WRITE to process
 *  mode:   Whether to read or write
 *
 *  Depending on specified MODE, either
 *  RWMode::READ    -- Reads specified PARAMS into msg->payload
 *  RWMode::WRITE   -- Writes specified PARAMS from msg->payload into devices
 */
void Device::device_rw_all (message_t *msg, RWMode mode)
{
    uint32_t param_bitmap;
    if (mode == RWMode::READ) {
        // Read all subscribed params
        param_bitmap = this->params;

        msg->message_id = MessageID::DEVICE_DATA;

        // Set beginning of payload to subscribed param bitmap
        uint32_t* payload_ptr_uint32 = (uint32_t *) msg->payload;
        *payload_ptr_uint32 = this->params;
        // *((uint32_t*) msg->payload) = this->params; TODO: Could be used to replace above two lines if valid syntax
        msg->payload_length = PARAM_BITMAP_BYTES;

    } else if (mode == RWMode::WRITE) {
        // Param bitmap of parameters to write is at the beginning of the payload
        param_bitmap = *((uint32_t*) msg->payload);
    }

    // Loop over param_bitmap and attempt to read or write data for requested bits
    for (uint32_t param_num = 0; (param_bitmap >> param_num) > 0; param_num++) {
        if (param_bitmap & (1 << param_num)) {
            if (mode == RWMode::READ) { // Append the value of the parameter to payload
                msg->payload_length += device_read((uint8_t) param_num,                     \
                                                    &(msg->payload[msg->payload_length]),   \
                                                    (size_t) sizeof(msg->payload) - msg->payload_length);
            } else if (mode == RWMode::WRITE){ // Write the next parameter to the device
                device_write((uint8_t) param_num, &(msg->payload[msg->payload_length]));
            }
        }
    }
}
