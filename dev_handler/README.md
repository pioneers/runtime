
# Device Handler

The device handler (abbreviated "dev handler" in the source code) is responsible for handling communication between shared memory and the Arduino ("lowcar") devices connected to the robot.

## Building and Usage
There are no third-party dependencies! The device handler needs shared memory to be running in the background first. (See shared memory README) Afterward, compile using `make` and run with `./dev_handler`.

The logs should indicate that the device handler has started polling. At this point, you can connect (or disconnect) USB devices.

## Main Routine

 The device handler constantly polls for newly connected devices and spawns (1) a **relayer** thread, (2) a **sender** thread, and (3) a **receiver** thread to act on the new devices.

1. The **relayer** verifies that the device is a lowcar device and connects it to shared memory. The relayer will then signal the **sender** and **receiver** to begin work. Afterward, the relayer makes sure that the device handler is receiving continuous messages from the device.
If the device times out or disconnects, the relayer is responsible for cleaning up after all three threads and disconnecting the device from shared memory.

2. The **sender** has the responsibility of checking if shared memory has new data to be written to the device. The sender will package, serialize, and write the data to the serial port in the form of a `DEVICE_WRITE` message. The sender also sends `SUBSCRIPTION_REQUEST` messages and periodic `PING` messages to the device.

3. The **receiver** continuously attempts to parse incoming data from the device and takes action based on the type of message received. This means updating shared memory with new device data in `DEVICE_DATA` messages and sending `LOG` messages to the logger.
