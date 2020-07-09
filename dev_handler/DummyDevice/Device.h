/**
 * Class defining a lowcar device, receiving/sending messages and performing actions based on received messages.
 */

#ifndef DEVICE_H
#define DEVICE_H

#include "defs.h"
#include "Messenger.h"
#include "StatusLED.h"

class Device
{
public:
  //******************************************* UNIVERSAL DEVICE METHODS ************************************** //
  /* Constructor with default args (times in ms)
   * set timeout to 0 if no disable on lack of PING
   * set ping_interval to 0 if don't PING
   * calls device_enable to enable the device
   * dev_id and dev_year are the device type and device year of the device
   */
  Device (DeviceType dev_type, uint8_t dev_year, uint32_t timeout = 1000, uint32_t ping_interval = 200);

  /* Generic device loop function that wraps all device actions
   * asks Messenger to read a new packet, if any, and responds appropriately
   * sends heartbeat requests and device data at set intervals
   */
  void loop ();

  //******************************************* DEVICE-SPECIFIC METHODS ************************************** //
  /* These functions are meant to be overridden by overridden by each device as it sees fit.
   * There are default dummy implementations of all these functions that do nothing so that the program will not
   * crash if they are not overwritten (for example, you don't need to overwrite device_write for a device that
   * has only read-only parameters).
   */

  /* This function is called when the device receives a Device Read packet.
   * It modifies DATA_BUF to contain the most recent value of parameter PARAM.
   * param      -   Parameter index (0, 1, 2, 3 ...)
   * data_buf     -   Buffer to return data in, little-endian
   * buf_len      -   Number of bytes available in data_buf to store data
   *
   * return     -   sizeof(<parameter_value>) on success; 0 otherwise
   */
  virtual uint8_t device_read (uint8_t param, uint8_t *data_buf, size_t data_buf_len);

  /* This function is called when the device receives a Device Write packet.
   * Updates PARAM to new value contained in DATA.
   * param      -   Parameter index (0, 1, 2, 3 ...)
   * data_buf     -   Contains value to write, little-endian
   *
   * return     -   sizeof(<bytes_written>) on success; 0 otherwise
   */
  virtual uint32_t device_write (uint8_t param, uint8_t *data_buf);

  /* This function is called in the Device constructor
   * (or potentially on receiving a Device Enable packet in the future).
   * It should do whatever setup is necessary for the device to operate.
   */
  virtual void device_enable ();

  /* This function is called when receiving a Device Disable packet, or
   * when the controller has stopped responding to heartbeat requests.
   * It should do whatever cleanup is necessary for the device to disable.
   */
  virtual void device_disable ();

  /* This function is called each time the device goes through the main loop.
   * Any continuously updating actions the device needs to do should be placed
   * in this function.
   * IMPORTANT: This function should not block!
   */
  virtual void device_actions ();

private:
  //******************************* PRIVATE VARIABLES AND HELPER METHOD ************************************** //
  const static float MAX_SUB_DELAY_MS;  //maximum tolerable subscription delay, in ms
  const static float MIN_SUB_DELAY_MS;  //minimum tolerable subscription delay, in ms
  const static float ALPHA;       //subscription delay interpolation tuning constant

  Messenger *msngr;   // Encodes/decodes and send/receive messages over serial
  StatusLED *led;     // The LED on the Arduino
  dev_id_t dev_id;      // dev_id of this device determined when flashing
  uint32_t params;    // Bitmap of parameters subscribed to by dev handler
  uint16_t sub_interval;  // Time between sending new DEVICE_DATA messages
  uint32_t timeout;   // Maximum time (ms) we'll wait between PING messages from dev handler
  uint32_t ping_interval; // Time (ms) between each PING message we send out
  uint64_t last_sub_time;     // Timestamp of last time we sent DEVICE_DATA due to Subscription
  uint64_t last_sent_ping_time; // Timestamp of last time we sent a PING
  uint64_t last_received_ping_time; // Timestamp of last time we received a PING
  uint64_t curr_time;       // The current time
  message_t curr_msg;       //current message being processed
  bool acknowledged;    // Whether or not we sent an ACKNOWLEDGEMENT

  //read or write data to device (more detail in source file)
  void device_rw_all (message_t *msg, uint32_t params, RWMode mode);
};

#endif
