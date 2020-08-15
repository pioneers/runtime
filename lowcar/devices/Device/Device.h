/*
 * Class defining a lowcar device, receiving/sending messages and performing actions based on received messages.
 */

#ifndef DEVICE_H
#define DEVICE_H

#include "defs.h"
#include "Messenger.h"
#include "StatusLED.h"

class Device {

public:
	//******************************************* UNIVERSAL DEVICE METHODS ************************************** //

	/* Constructor with default args (times in ms)
	 * set timeout to 0 if no disable on lack of PING
	 * set ping_interval to 0 if don't PING
	 * calls device_enable to enable the device
	 * dev_id and dev_year are the device type and device year of the device
	 */
	Device (DeviceType dev_type, uint8_t dev_year, uint32_t timeout = 1000, uint32_t ping_interval = 250);

	/* Sets the UID of the Device */
	void set_uid (uint64_t uid);

	/* Generic device loop function that wraps all device actions
	 * Asks messenger to read any incoming messages and responds appropriately
	 * Sends PING and DEVICE_DATA at specified interval
	 * Sends log messages if any
	 * calls device_actions() to do device-type-specific actions
	 */
	void loop ();

	//************************************** DEVICE-SPECIFIC METHODS ************************************** //
	/* These functions are meant to be overridden by overridden by each device as it sees fit.
	* There are default dummy implementations of all these functions that do nothing so that the program will not
	* crash if they are not overwritten (for example, you don't need to overwrite device_write for a device that
	* has only read-only parameters).
	*/

	/* This function is called periodically to build a DEVICE_DATA message
	 * It modifies DATA_BUF to contain the most recent value of parameter PARAM.
	 * param      -   Parameter index (0, 1, 2, 3 ...)
	 * data_buf   -   Buffer to return data in, little-endian
	 * buf_len    -   Number of bytes available in data_buf to store data
	 *
	 * return     -   sizeof(<parameter_value>) on success; 0 otherwise
	 */
	virtual size_t device_read (uint8_t param, uint8_t *data_buf);

	/* This function is called when the device receives a DEVICE_WRITE message
	 * Updates PARAM to new value contained in DATA_BUF.
	 * param      -   Parameter index (0, 1, 2, 3 ...)
	 * data_buf   -   Contains value to write, little-endian
	 *
	 * return     -   sizeof(<parameter_value>) on success; 0 otherwise
	 */
	virtual size_t device_write (uint8_t param, uint8_t *data_buf);

	/* This function is called in the Device constructor
	 * It should do whatever setup is necessary for the device to operate.
	 */
	virtual void device_enable ();

	/* This function is called when the device hasn't received a PING message
	 * from dev handler for the specified timeout duration.
	 * It should do whatever cleanup is necessary for the device to disable.
	 */
	virtual void device_disable ();

	/* This function is called each time the device goes through the main loop.
	 * Any continuously updating actions the device needs to do should be placed
	 * in this function.
	 * IMPORTANT: This function should not block!
	 */
	virtual void device_actions ();

protected:
    Messenger *msngr;                 // Encodes/decodes and send/receive messages over serial
    uint8_t enabled;

private:
	const static float MAX_SUB_INTERVAL_MS;  // Maximum tolerable subscription delay, in ms
	const static float MIN_SUB_INTERVAL_MS;  // Minimum tolerable subscription delay, in ms

	StatusLED *led;                   // The LED on the Arduino
	dev_id_t dev_id;                  // dev_id of this device determined when flashing
	uint32_t params;                  // Bitmap of parameters subscribed to by dev handler
	uint16_t sub_interval;            // Time between sending new DEVICE_DATA messages
	uint32_t timeout;                 // Maximum time (ms) we'll wait between PING messages from dev handler
	uint64_t last_sent_data_time;     // Timestamp of last time we sent DEVICE_DATA due to Subscription
	uint64_t last_received_ping_time; // Timestamp of last time we received a PING
	uint64_t curr_time;               // The current time
	message_t curr_msg;               // current message being processed

	//Builds a DEVICE_DATA message by reading all subscribed params
	void device_read_params (message_t *msg);

	//Writes to device parameters using input MSG of type DEVICE_WRITE
	void device_write_params (message_t *msg);
};

#endif
