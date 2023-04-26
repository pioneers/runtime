#include "PDB.h"

// this is the length of the INVERTED_7SEG_CHARS array
#define NUM_UNIQUE_DISPLAY_CHARS 19

const uint8_t PDB::INVERTED_7SEG_CHARS[][2] = {
    {0b11101011, '0'},  // 0
    {0b01100000, '1'},  // 1
    {0b01011011, '2'},  // 2
    {0b01111010, '3'},  // 3
    {0b11110000, '4'},  // 4
    {0b10111010, '5'},  // 5
    {0b10111011, '6'},  // 6
    {0b11101000, '7'},  // 7
    {0b11111011, '8'},  // 8
    {0b11111010, '9'},  // 9

    {0b11111001, 'A'},  // A
    {0b10011001, 'F'},  // F
    {0b10000011, 'L'},  // L
    {0b10011011, 'E'},  // E
    {0b10001011, 'C'},  // C
    {0b11100011, 'U'},  // U
    {0b11011001, 'P'},  // P
    {0b11101001, 'N'},  // N
    {0b00000000, ' '}   // blank
};

/* 
 * Bit correspondence to display:
 * |  x  |  x  |  x  |  x  |  x  |  x  |  x  |  x  |
 * | top | top | bot | mid | top | pt. | bot | bot |
 * | left|right|right|     |     |     |     | left|
 */

// Here, we call the Device constructor with Serial1 as the argument for
// Serial port because we want the PDB to use the Serial1 port, not the Serial port (micro USB)
PDB::PDB() : Device(DeviceType::PDB, 1, true, &Serial1) {
    this->v_cell1 = 0;     // param
    this->v_cell2 = 0;     // param 4
    this->v_cell3 = 0;     // param 5
    this->v_batt = 0;      // param 6
    this->vref_guess = 0;  // param 9
    this->sequence = 0;

    this->digitPins[0] = DISP_PIN_1;
    this->digitPins[1] = DISP_PIN_2;
    this->digitPins[2] = DISP_PIN_3;
    this->digitPins[3] = DISP_PIN_4;
    this->last_seg_change = this->last_measure_time = this->curr_time;

    pinMode(digitPins[0], OUTPUT);
    pinMode(digitPins[1], OUTPUT);
    pinMode(digitPins[2], OUTPUT);
    pinMode(digitPins[3], OUTPUT);

    pinMode(CELL1, INPUT);
    pinMode(CELL2, INPUT);
    pinMode(CELL3, INPUT);

    pinMode(NET_SWITCH_PIN, INPUT);  // network switch
    pinMode(BUZZER, OUTPUT);

    pinMode(TX, OUTPUT);
    pinMode(RX, INPUT);

    Wire.begin();
}

size_t PDB::device_read(uint8_t param, uint8_t* data_buf) {
    // Float params
    float* float_buf = (float*) data_buf;
    switch (param) {
        // Boolean params
        case IS_UNSAFE:
            data_buf[0] = FALSE;  // is_unsafe();
            return sizeof(uint8_t);
        case CALIBRATED:
            data_buf[0] = TRUE;  // auto set to true, not using calibrations right now, maybe fix later
            return sizeof(uint8_t);
        case V_CELL1:
            float_buf[0] = this->v_cell1;
            break;
        case V_CELL2:
            float_buf[0] = this->v_cell2;
            break;
        case V_CELL3:
            float_buf[0] = this->v_cell3;
            break;
        case V_BATT:
            float_buf[0] = this->v_batt;
            break;
        case DV_CELL2:
            float_buf[0] = 0.0;
            break;
        case DV_CELL3:
            float_buf[0] = 0.0;
            break;
        case NETWORK_SWITCH:
            data_buf[0] = digitalRead(NET_SWITCH_PIN);
            return sizeof(uint8_t);
    }
    return sizeof(float);
}

void PDB::device_enable() {
    // set pins, clear display, obtain calibrations?
    // pinMode(digitPins[0], OUTPUT);
    // pinMode(digitPins[1], OUTPUT);
    // pinMode(digitPins[2], OUTPUT);
    // pinMode(digitPins[3], OUTPUT);

    // pinMode(CELL1, INPUT);
    // pinMode(CELL2, INPUT);
    // pinMode(CELL3, INPUT);

    // pinMode(NET_SWITCH_PIN, INPUT);  // network switch
    // pinMode(BUZZER, OUTPUT);

    // pinMode(TX, OUTPUT);
    // pinMode(RX, INPUT);

    // Wire.begin();  // SDA / SCL for expander
}

void PDB::device_actions() {
    handle_8_segment();

    if ((this->curr_time - this->last_measure_time) > 1500) {
        measure_cells();
        this->last_measure_time = this->curr_time;
        // handle_safety();    don't buzz, change back when voltages are working again
    }
}

void PDB::measure_cells() {
    int r_cell1 = analogRead(CELL1);
    int r_cell2 = analogRead(CELL2);
    int r_cell3 = analogRead(CELL3);

    this->v_cell1 = float(r_cell1) * 4.0 / float(ADC_COUNTS) * 5.0;
    this->v_cell2 = (float(r_cell2) * 4.0 / float(ADC_COUNTS) * 5.0) - this->v_cell1;
    this->v_cell3 = (float(r_cell3) * 4.0 / float(ADC_COUNTS) * 5.0) - this->v_cell2 - this->v_cell1;
    if (this->v_cell1 < 3 || this->v_cell2 < 3 || this->v_cell3 < 3) {
        tone(BUZZER, NOTE_A5);
    } else {
        noTone(BUZZER);
    }

    this->v_batt = this->v_cell1 + this->v_cell2 + this->v_cell3;
}

void PDB::handle_8_segment() {
    switch (this->sequence) {
        case 0:
            this->writeString("ALL");
            break;
        case 1:
            this->writeFloat(this->v_batt);
            break;
        case 2:
            this->writeString("CEL1");
            break;
        case 3:
            this->writeFloat(this->v_cell1);
            break;
        case 4:
            this->writeString("CEL2");
            break;
        case 5:
            this->writeFloat(this->v_cell2);
            break;
        case 6:
            this->writeString("CEL3");
            break;
        case 7:
            this->writeFloat(this->v_cell3);
            break;
        case 8:
            this->writeString("0N"); // Supposed to be "On" for "On network:" -> display the network status
            break;
        case 9:
            if (digitalRead(NET_SWITCH_PIN) == false) {
                this->writeString("P1E"); // Supposed to be "PiE" -> pioneers or Motherbase
            } else {
                this->writeString("L0C"); // Supposed to be "LOC" for "Local" -> Team router
            }
            break;
    }

    if (this->curr_time > (this->last_seg_change + 1000)) {  // every second
        this->sequence = this->sequence + 1;
        if (this->sequence == 10) {
            this->sequence = 0;
        }
        this->last_seg_change = this->curr_time;
    }
    // this->writeString("CEL1");
    // this->msngr->lowcar_printf("case 0");
}

void PDB::clearDisp() {             // reset all the pins so none of the digits are displayed
    digitalWrite(DISP_PIN_1, LOW);  // Set all back to LOW after
    digitalWrite(DISP_PIN_2, LOW);
    digitalWrite(DISP_PIN_3, LOW);
    digitalWrite(DISP_PIN_4, LOW);
    expanderWrite(0b11111111);
}

void PDB::expanderWrite(byte data) {  // write the given byte data to the display //this DISPLAYS
    Wire.beginTransmission(expander);
    Wire.write(data);
    Wire.endTransmission();
}

void PDB::writeString(char* str) {
    uint8_t bytearray[strlen(str)];
    int index = 0;
    for (int i = 0; i < strlen(str); i++) {
        for (int j = 0; j < NUM_UNIQUE_DISPLAY_CHARS; j++) {
            if (PDB::INVERTED_7SEG_CHARS[j][1] == str[i]) {
                uint8_t justChar = PDB::INVERTED_7SEG_CHARS[j][0];
                if (i == strlen(str) - 1) {
                    bytearray[index] = justChar;
                    index++;
                } else if (str[i + 1] == '.') {
                    bytearray[index] = (0b00000100 | justChar);
                    index++;
                    i++;
                } else {
                    bytearray[index] = justChar;
                    index++;
                }
            }
        }
    }
    // debugging notes: changed byte array size to strlen(str)
    for (int k = 0; k < strlen(str) && k < 4; k++) {
        digitalWrite(digitPins[k], HIGH);
        expanderWrite(~bytearray[k]);
        delay(1);
        clearDisp();  // digitalWrite(digitPins[k], LOW);
    }
}

void PDB::writeFloat(float val) {
    char str[6];
    int integer_part = (int) val;
    int decimal_part = ((int) (val * 100.0)) % 100;
    // int decimal_part = (int) ((val - integer_part) * 100); # changes 8 to 7 at end of value

    if (integer_part < 10) {
        sprintf(str, " %d.%02d", integer_part, decimal_part);  // this sets first display to empty if only 3 sig figs
    } else {
        sprintf(str, "%d.%02d", integer_part, decimal_part);  // %02d adds a zero in front if decimal_part is less than 2 digits
    }
    writeString(str);
}
