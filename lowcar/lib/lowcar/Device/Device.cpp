#include "Device.h"

const float Device::MAX_SUB_INTERVAL_MS = 500.0;    // maximum tolerable subscription delay, in ms
const float Device::MIN_SUB_INTERVAL_MS = 40.0;     // minimum tolerable subscription delay, in ms

//Device constructor
Device::Device (DeviceType dev_id, uint8_t dev_year, uint32_t timeout, uint32_t ping_interval)
{
    this->dev_id.type = dev_id;
    this->dev_id.year = dev_year;
    this->dev_id.uid = 0xFEDCBA987654321; // TODO: Flashing must set this to a random uint64_t

    this->params = 0;         // nothing subscribed to right now
    this->sub_interval = 0;   // 0 acts as flag indicating no subscription
    this->timeout = timeout;
    this->ping_interval = ping_interval;
    this->acknowledged = false;

    this->msngr = new Messenger();
    this->led = new StatusLED();

    device_enable(); //call device's enable function

    this->last_sub_time = this->last_sent_ping_time = this->last_received_ping_time = this->curr_time = millis();
}

//universal loop function
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
                // If this is the first PING received, send an ACKNOWLEDGEMENT
                if (!this->acknowledged) {
                    this->msngr->lowcar_printf("Device type %d with UID ending in %X contacted; sending ACK", this->dev_id.type, this->dev_id.uid);
                    this->msngr->send_message(MessageID::ACKNOWLEDGEMENT, &(this->curr_msg), &(this->dev_id));
                    this->acknowledged = true;
                }
                break;

            case MessageID::SUBSCRIPTION_REQUEST:
                this->params = *((uint32_t *) this->curr_msg.payload);    // Update subscribed params
                this->sub_interval = *((uint16_t *) (this->curr_msg.payload + PARAM_BITMAP_BYTES));  // Update the interval at which we send DEVICE_DATA
                // Make sure sub_interval is within bounds
                this->sub_interval = min(this->sub_interval, MAX_SUB_INTERVAL_MS);
                this->sub_interval = max(this->sub_interval, MIN_SUB_INTERVAL_MS);
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

    // If we still haven't gotten our first PING yet, keep waiting for it
    if (!(this->acknowledged)) {
        return;
    }

    /* Send another DEVICE_DATA with subscribed parameters if this->sub_interval
     * passed since the last time we sent a DEVICE_DATA
     * If this->sub_interval == 0, don't send DEVICE_DATA */
    if ((this->sub_interval > 0) && (this->curr_time - this->last_sub_time >= this->sub_interval)) {
		this->last_sub_time = this->curr_time;
        device_read_params(&(this->curr_msg));
        this->msngr->send_message(MessageID::DEVICE_DATA, &(this->curr_msg));
    }

    // Send another PING if this->ping_interval passed since the last time we sent a PING
    // Don't start sending PING messages until after we sent an ACKNOWLEDGEMENT
    if ((this->ping_interval > 0) && (this->curr_time - this->last_sent_ping_time >= this->ping_interval)) {
        this->last_sent_ping_time = this->curr_time;
        this->msngr->send_message(MessageID::PING, &(this->curr_msg));
    }

    // Send any queued logs
    this->msngr->lowcar_flush();

    // If it's been too long since we received a PING, disable the device
    if ((this->timeout > 0)  && (this->curr_time - this->last_received_ping_time >= this->timeout)) {
        device_disable();
		this->acknowledged = false;
    }

    device_actions(); //do device-specific actions
}

//************************************************ DEFAULT DEVICE-SPECIFIC METHODS ******************************************* //

//a device uses this function to return data about its state
size_t Device::device_read (uint8_t param, uint8_t *data_buf)
{
  return 0; //by default, we read 0 bytes into buffer
}

//a device uses this function to change a state
size_t Device::device_write (uint8_t param, uint8_t *data_buf)
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

void Device::device_read_params (message_t *msg)
{
    // Clear the message before building device data
    msg->message_id = MessageID::DEVICE_DATA;
    msg->payload_length = 0;
	memset(msg->payload, 0, MAX_PAYLOAD_SIZE);
	
    // Read all subscribed params
    // Set beginning of payload to subscribed param bitmap
    uint32_t* payload_ptr_uint32 = (uint32_t *) msg->payload;
    *payload_ptr_uint32 = this->params;
	
    // Loop over param_bitmap and attempt to read data for subscribed bits
	msg->payload_length = PARAM_BITMAP_BYTES;
    for (uint8_t param_num = 0; (this->params >> param_num) > 0; param_num++) {
        if (this->params & (1 << param_num)) {
            msg->payload_length += device_read(param_num, msg->payload + msg->payload_length);
		}
    }
	//this->msngr->lowcar_printf("params is %08X", *((uint32_t *)msg->payload));
}

void Device::device_write_params (message_t *msg)
{
	// Param bitmap of parameters to write is at the beginning of the payload
    uint32_t param_bitmap = *((uint32_t*) msg->payload);

    // Loop over param_bitmap and attempt to write data for requested bits
	uint8_t *payload_ptr = msg->payload + PARAM_BITMAP_BYTES;
    for (uint32_t param_num = 0; (param_bitmap >> param_num) > 0; param_num++) {
        if (param_bitmap & (1 << param_num)) {
			payload_ptr += device_write((uint8_t) param_num, payload_ptr);
		}
	}
}
