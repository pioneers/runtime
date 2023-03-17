/**
 * Class defining a lowcar device, receiving/sending messages
 * and performing actions based on received messages.
 */

#ifndef DEVICE_H
#define DEVICE_H

#include "Messenger.h"
#include "StatusLED.h"
#include "defs.h"

class Device {
  public:
    // ********************** UNIVERSAL DEVICE METHODS ********************** //

    /**
     *  Constructor with default args
     * Calls device_enable to enable the device.
     * Arguments:
     *    dev_type: The type of device (ex: LimitSwitch)
     *    dev_year: The device year
     *    timeout: the maximum number of milliseconds to wait between PING messages from
     *      dev handler before disabling
     *      It's reasonable to match the TIMEOUT that dev handler uses.
     */
    Device(DeviceType dev_type, uint8_t dev_year, uint32_t timeout = 1000);

    // Sets the UID of the Device
    void set_uid(uint64_t uid);

    /**
     * Generic device loop function that wraps all device actions.
     * Asks Messenger to read any incoming messages and responds appropriately.
     * Sends DEVICE_DATA at specified interval.
     * Sends log messages if any are queued.
     * Processes inocming DEVICE_WRITE messages.
     * Calls device_actions() to do device-type-specific actions.
     */
    void loop();

    // ********************** DEVICE-SPECIFIC METHODS *********************** //

    /* These functions are meant to be overridden by overridden by each device.
     * The default implementations do nothing.
     * For example, you don't need to overwrite device_write() for a device that
     * has only read-only parameters).
     */

    /**
     * Reads the value of a parameter into a buffer.
     * Helper function used to build a DEVICE_DATA message.
     * Arguments:
     *    param: The 0-indexed index of the parameter to read
     *    data_buf: The buffer to read the parameter value into
     * Returns:
     *    the size of the parameter read into the buffer, or
     *    0 on failure (ex: because the parameter is not readable)
     */
    virtual size_t device_read(uint8_t param, uint8_t* data_buf);

    /**
     * Writes the value of a parameter from a buffer into the device
     * Helper function used to process a DEVICE_WRITE message.
     * Arguments:
     *    param: The 0-indexed index of the parameter being written
     *    data_buf: Buffer containing the parameter value being written
     * Returns:
     *    the size of the parameter that was written, or
     *    0 on failure.
     */
    virtual size_t device_write(uint8_t param, uint8_t* data_buf);

    /**
     * Performs necessary setup for the device to operate.
     * Called in the Device constructor
     */
    virtual void device_enable();

    /**
     * Performs necessary cleanup to reset the device
     * Called when we want to reset device to initial state when first plugged in
     * and establishes a connection with dev_handler
     */
    virtual void device_reset();

    /**
     * The "main" function of a device, performing continuously updating actions.
     * This function SHOULD NOT block. It is called in each iteration of
     * Device::loop().
     * It performs continuous actions that the device should do
     * For example, the motor controller would move the motors
     */
    virtual void device_actions();

  protected:
    Messenger* msngr;  // Encodes/decodes and send/receive messages over serial
    uint8_t enabled;
    StatusLED* led;  // The LED on the Arduino

  private:
    dev_id_t dev_id;                   // dev_id of this device determined when flashing
    uint32_t timeout;                  // Maximum time (ms) we'll wait between PING messages from dev handler
    uint64_t last_sent_data_time;      // Timestamp of last time we sent DEVICE_DATA
    uint64_t last_received_ping_time;  // Timestamp of last time we received a PING
    uint64_t curr_time;                // The current time
    message_t curr_msg;                // current message being processed

    /**
     * Builds a DEVICE_DATA message by reading all readable parameters.
     * Arguments:
     *    msg: An empty message to be populated with parameter values ready for sending.
     */
    void device_read_params(message_t* msg);

    /**
     * Writes to device parameters given a DEVICE_WRITE message.
     * Arguments:
     *    msg: A DEVICE_WRITE message containing parameters to write to the device.
     */
    void device_write_params(message_t* msg);
};

#endif
