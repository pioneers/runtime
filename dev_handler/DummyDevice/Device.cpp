#include "Device.h"

const float Device::MAX_SUB_DELAY_MS = 250.0;  //maximum tolerable subscription delay, in ms
const float Device::MIN_SUB_DELAY_MS = 40.0;  //minimum tolerable subscription delay, in ms
const float Device::ALPHA = 0.25;   //tuning parameter for how the interpolation for updating subscription delay should happen


//Device constructor
Device::Device (DeviceType dev_id, uint8_t dev_year, uint32_t timeout, uint32_t ping_interval)
{
  this->dev_id.type = dev_id;
  this->dev_id.year = dev_year;
  this->dev_id.uid = 0x0123456789ABCDEF;

  this->params = 0;     // nothing subscribed to right now
  this->sub_interval = 0; // 0 acts as flag indicating no subscription
  this->timeout = timeout;
  this->ping_interval = ping_interval;
  this->last_sub_time = this->last_sent_ping_time = this->last_received_ping_time = this->curr_time = millis();
  this->acknowledged = false;

  // Initialize the Messenger; Putting msngr() in initializer list doesn't run constructor properly
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
  uint32_t *payload_ptr_uint32; //use this to shove 32 bits into the first four elements of the payload (which is of type uint8_t *)

  this->curr_time = millis();
  sts = this->msngr->read_message(&(this->curr_msg)); //try to read a new message
  this->led->slow_blink(3);
  if (sts == Status::SUCCESS) { //we have a message!
    switch (this->curr_msg.message_id) {
      case MessageID::PING:
        this->last_received_ping_time = this->curr_time;
        this->led->quick_blink(4);
        if (!this->acknowledged) {
          this->led->quick_blink(5);
          if(this->msngr->send_message(MessageID::ACKNOWLEDGEMENT, &(this->curr_msg), this->params, this->sub_interval, &(this->dev_id)) == Status::PROCESS_ERROR){
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
        this->sub_interval = min(this->sub_interval, MAX_SUB_DELAY_MS);
        this->sub_interval = max(this->sub_interval, MIN_SUB_DELAY_MS);
        break;

      case MessageID::DEVICE_WRITE:
        //attempt to write specified specified params to device; set payload[0:2] to successfully written params
        payload_ptr_uint32 = (uint32_t *) this->curr_msg.payload; //store pointer to the front of the payload, cast to uint32_t
        device_rw_all(&(this->curr_msg), *(uint8_t *)payload_ptr_uint32, RWMode::WRITE);  //payload_ptr_uint32 contains bitmap for rw
        break;

      // Receiving some other Message
      //default:
        //this->led->toggle();
    }
  } else {
    // No message received
  }

  // Send another DEVICE_DATA with subscribed parameters if it's time
  if ((this->sub_interval > 0) && (this->curr_time - this->last_sub_time >= this->sub_interval)) {
    this->last_sub_time = this->curr_time;
    // Append the subscribed param bitmap to the beginning payload
    payload_ptr_uint32 = (uint32_t *) this->curr_msg.payload; //store the pointer to the front of the payload, cast to uint32_t
    *payload_ptr_uint32 = this->params;
    // Append the data of each subscribed param to the payload
    device_rw_all(&(this->curr_msg), this->params, RWMode::READ);
    this->msngr->send_message(MessageID::DEVICE_DATA, &(this->curr_msg));
  }

  // Send another PING if it's time
  if ((this->acknowledged) && (this->ping_interval > 0) && (this->curr_time - this->last_sent_ping_time >= this->ping_interval)) {
    //this->led->quick_blink(7);
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

//************************************************************* HELPER METHOD *************************************************** //

/* Takes in a message_t to work with, a param bitmap, and whether to write to or read from the device.
 * on RWMode::READ -- attempts to read specified params into msg->payload, in order (for DEVICE_DATA)
 * on RWMode::WRITE -- attemptss to write all specified params from msg->payload to device (for DEVICE_WRITE)
 * In both cases, msg->payload_length set to number of bytes successfully written (to msg->payload or to the device).
 */
void Device::device_rw_all (message_t *msg, uint32_t params, RWMode mode)
{
  int bytes_written = PARAM_BITMAP_BYTES; // DEVICE_DATA requires a pmap at the beginning

  //loop over params and attempt to read or write data for requested bits
  for (uint32_t param_num = 0; (params >> param_num) > 0; param_num++) {
    if (params & (1 << param_num)) {
      if (mode == RWMode::READ) { //read parameter into payload at next available bytes in payload; returns # bytes read (= # bytes written to payload)
        bytes_written += device_read((uint8_t) param_num, &(msg->payload[bytes_written]), (size_t) sizeof(msg->payload) - bytes_written);
      } else if (mode == RWMode::WRITE){ //write parameter to device; returns # bytes written
        bytes_written += device_write((uint8_t) param_num, &(msg->payload[bytes_written]));
      }
    }
  }
  msg->payload_length = bytes_written; //put how many bytes we wrote to as payload length
}
