#ifndef RFID_H
#define RFID_H

#include "Device.h"
#include "defs.h"
#include <SPI.h>
#include <MFRC522.h>

//give params more readable names
enum class RFID_Param : uint8_t {
	ID 			= 0,
	TAG_DETECT 	= 2		//is 1 if we have a tag; 0 otherwise
};

class RFID : public Device {
public:
	//constructs an RFID; simply calls generic Device constructor with device type and year
	//instatiates the mfrc522 private variable, and initializes the other private variables
	RFID ();

	//overridden functions from Device class; see descriptions in Device.h
	virtual size_t device_read (uint8_t param, uint8_t *data_buf);
	virtual void device_enable ();
	virtual void device_actions ();

private:
	//Pin definitions
	const static int RST_PIN;
	const static int SS_PIN;

	MFRC522 *tag_detector; //object used to operate the actual RFID tag detector
	uint8_t delay; //for delaying the reading of the device

	// Outward facing (read-only) params
	uint32_t id;
	uint8_t tag_detect; //tag_detect parameter
};

#endif
