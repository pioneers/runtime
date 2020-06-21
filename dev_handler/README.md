
# Device Handler

The device handler (abbreviated "dev handler" in the source code) is responsible for handling communication between shared memory and the lower-level Arduino devices (nicknamed "lowcar" devices).

## Main Routine

 - The device handler constantly polls for newly connected devices and spawns a     **relayer** thread, a **sender** thread, and a **receiver** thread to act on the new devices.

 -  The **relayer** verifies that the device is a lowcar device and connects it to shared memory. The relayer will then signal the **sender** and **receiver** to begin work. Afterwards, the relayer continuously makes sure that all heartbeat requests sent from the **sender** are resolved in a timely manner.

 If the device times out or disconnects, the relayer is responsible for cleaning up after all three threads and disconnecting the device from shared memory
 - The **sender** has the responsibility of checking if shared memory has new data to be written to the device. The sender will serialize and bulk transfer the data over USB. The sender also periodically sends heartbeat requests to the device and resolves any received heartbeat requests noted by the **receiver**.
 - The **receiver** continuously attempts to parse incoming data from the device and takes action based on the type of message received. This includes signaling that a heartbeat request/response was received to the other two threads, updating shared memory with new device data, and logging messages.

## Installation
The only third-party library used by the device handler is libusb
libusb provides functionality for C code to perform I/O on USB devices.
Regardless of whether you are using Linux or Mac, use the library flags
`-lusb-1.0 -lpthread`, as the device handler also uses pthreading (to spawn the relayer, sender, and receiver threads).

### Linux
libusb can be installed on Linux using
```bash
sudo apt-get install libusb-1.0-0-dev
```
Compile using CFLAG `-I/usr/include/libusb-1.0`
### Mac
Compile using CFLAG `-I/usr/local/include/libusb-1.0` and
LDFLAG  `-L/usr/local/lib/`
## Usage
The Makefile should already be configured to handle compilation.

Compile and run the device handler by using
```bash
make dev
```
The terminal should indicate that the device handler has started polling. At this point, you can connect (or disconnect) USB devices.
Alternatively,
```bash
make val
```
runs the device handler with Valgrind to detect memory leaks and invalid reads/writes. This *make* target is purely for debugging purposes.
