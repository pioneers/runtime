#include "RFID.h"

const int RFID::RST_PIN = 9;
const int RFID::SS_PIN = 10;

// default constructor simply specifies DeviceID and year to generic constructor and initializes variables
// initializes the msfrc22 object in the initializer list
RFID::RFID() : Device(DeviceType::RFID, 1), tag_detector(RFID::SS_PIN, RFID::RST_PIN) {
    this->id_upper = 0;
    this->id_lower = 0;
    this->tag_detect = 0;
    this->delay = FALSE;
}

size_t RFID::device_read(uint8_t param, uint8_t* data_buf) {
    switch ((RFID_Param) param) {  // switch the incoming parameter (cast to RFID_Param)
        case RFID_Param::ID:
            ((int32_t*) data_buf)[0] = this->id;
            return sizeof(this->id);

        case RFID_Param::TAG_DETECT:
            data_buf[0] = this->tag_detect;
            return sizeof(this->tag_detect);

        default:
            return 0;
    }
}

void RFID::device_enable() {
    SPI.begin();                     // begin SPI (what's this?)
    this->tag_detector->PCD_Init();  // initialize the RFID object
}

void RFID::device_actions() {
    /* If we either don't sense a card or can't read the UID, we invalidate the data
     * The sensor is too slow, so we have to delay the read by one loop
     * The delay makes sure that id and tag_detect don't update for
     * one cycle of the loop after finding a tag
     */
    if (!this->tag_detector->PICC_IsNewCardPresent() || !this->tag_detector->PICC_ReadCardSerial()) {
        if (this->delay) {
            this->id = 0;          // clear the id to invalidate it
            this->tag_detect = 0;  // no tag detected
        }
        this->delay = TRUE;
        return;  // after resetting all our values, we return
    }

    // Otherwise, if there is a card that we can read the UID from, we grab the data
    // and set tag_detect to 1 to signal that we have a new tag UID
    uint32_t uid = (uint32_t) (this->tag_detector->uid.uidByte[2]) << 16 |
                   (uint32_t) (this->tag_detector->uid.uidByte[1]) << 8 |
                   (uint32_t) (this->tag_detector->uid.uidByte[0]);
    this->id = uid;
    this->tag_detect = 1;
    this->delay = FALSE;  // reset the delay
}
