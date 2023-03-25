#include "PDB.h"
#include "pdb_defs.h"

// *********** PDB ***********
// PDB is both the old battery buzzer and wifi switch tracker

// default constructor simply specifies DeviceID and and year to generic constructor
// initalizes helper class objects and pre-existing variables
PDB::PDB() : Device(DeviceType::PDB, 1) {
    this->v_cell1 = 0;     // param
    this->v_cell2 = 0;     // param 4
    this->v_cell3 = 0;     // param 5
    this->v_batt = 0;      // param 6
    this->vref_guess = 0;  // param 9
    this->unsafe_status = FALSE;

    this->triple_calibration = TRUE;

    analogReference(DEFAULT);

    //this->buzzer = 10;
    this->last_check_time = 0;  //for loop counter

    this->digitPins[0] = DISP_PIN_1;
    this->digitPins[1] = DISP_PIN_2;
    this->digitPins[2] = DISP_PIN_3;
    this->digitPins[3] = DISP_PIN_4;

    this->last_LED_time = millis();  //Time the last LED switched
    this->sequence = 0;       //used to switch states for the display.  Remember that the hangle_8_segment cannot be blocking.

    this->segment_8_run = NORMAL_VOLT_READ;  //0 for the normal voltage readout.  1 for "Clear Calibration".  2 for "New Calibration"

    this->disp = new Disp();
}

size_t PDB::device_read(uint8_t param, uint8_t* data_buf) {
    // Float params
    float* float_buf = (float*) data_buf;
    switch (param) {
        // Boolean params
        case IS_UNSAFE:
            data_buf[0] = FALSE; // is_unsafe();
            return sizeof(uint8_t);
        case CALIBRATED:
            data_buf[0] = TRUE; // auto set to true, not using calibrations right now, maybe fix later
            return sizeof(uint8_t);
        case V_CELL1:
            float_buf[0] = v_cell1;
            break;
        case V_CELL2:
            float_buf[0] = v_cell2;
            break;
        case V_CELL3:
            float_buf[0] = v_cell3;
            break;
        case V_BATT:
            float_buf[0] = v_batt;
            break;
        case DV_CELL2:
            float_buf[0] = dv_cell2;
            break;
        case DV_CELL3:
            float_buf[0] = dv_cell3;
            break;
        case NETWORK_SWITCH:
            data_buf[0] = digitalRead(TOGGLE);
            return sizeof(uint8_t);
    }
    return sizeof(float);
}

void PDB::device_enable() {
	// set pins, clear display, obtain calibrations?
    pinMode(digitPins[0], OUTPUT);  
    pinMode(digitPins[1], OUTPUT);  
    pinMode(digitPins[2], OUTPUT);  
    pinMode(digitPins[3], OUTPUT); 

    pinMode(CELL1, INPUT);
    pinMode(CELL2, INPUT);
    pinMode(CELL3, INPUT);
  
    pinMode(BUZZER, OUTPUT);

    pinMode(TOGGLE, INPUT); // network switch

    pinMode(TX, OUTPUT);
    pinMode(RX, INPUT);
  
    Wire.begin(); // SDA / SCL for expander
}

void PDB::device_actions() {
    //setup_display();
    // disp->clearDisp();

    handle_8_segment();
    //SEQ_NUM mode = handle_calibration(); // calibrations and EEPROM
    //setup_sensing();

    if ((millis() - last_check_time) > 250) {
        measure_cells();
        last_check_time = millis();
        //handle_safety();    don't buzz, change back when voltages are working again
    }
    // disp->clearDisp();
}

// function that determines whether this PDB has been calibrated or not
uint8_t PDB::is_calibrated() {
    //triple calibration arrays are all default values
    if (triple_calibration && (calib[0] == ADC_REF_NOM) && (calib[1] == ADC_REF_NOM) && (calib[2] == ADC_REF_NOM)) {
        return FALSE;
    }
    //calibration value is set to default
    if (!triple_calibration && (get_calibration() == -1.0)) {
        return FALSE;
    }
    // device is calibrated: no default values
    return TRUE;
}

// void PDB::setup_display() {
//     disp->setDigitPins(numOfDigits, digitPins);
//     disp->setDPPin(DECIMAL_POINT);  //set the Decimal Point pin to #1
// }

/**
 * a function to ensure that all outputs of the display are working.
 * On the current PDB, this should display "8.8.8.8."
 * The library is capable of handling : and ', but the wires are not hooked up to it.
 */
void PDB::test_display() {
    // for (int i = 0; i < 250; i++) {
    //     //this function call takes a surprising amount of time,
    //     //probably because of how many characters are in it.
    //     disp->write(Disp::DisplayChars(string("8.8.8.8.")));
    // }
    disp->writeString("8.8.8.8.");
    disp->clearDisp();
}

/**
 * handles the 8-segment display, and prints out the global values.
 * MUST be called in the loop() without any if's
 */
void PDB::handle_8_segment() {
    if (segment_8_run == NORMAL_VOLT_READ) {
        //Shows text / numbers of all voltages & then individual cells.
        //Changed every Second
        switch (sequence) {
            case 0:
                disp->writeString("ALL");
                this->msngr->lowcar_printf("case 0");
                break;
            case 1:
                disp->writeFloat(v_batt);
                this->msngr->lowcar_printf("case 1");
                break;
            case 2:
                disp->writeString("CEL1");
                break;
            case 3:
                disp->writeFloat(v_cell1);
                break;
            case 4:
                disp->writeString("CEL2");
                break;
            case 5:
                disp->writeFloat(v_cell2);
                break;
            case 6:
                disp->writeString("CEL3");
                break;
            case 7:
                disp->writeFloat(v_cell3);
                break;
        }

        if (millis() > (last_LED_time + 1000)) {  //every second
            sequence = sequence + 1;
            if (sequence == 8) {
                sequence = 0;
            }
            last_LED_time = millis();
        }
    } else if (segment_8_run == CLEAR_CALIB) {
        if (sequence == 0) {
            disp->writeString("CAL");
        } else if (sequence == 1) {
            disp->writeString("CLR");
        }

        if (millis() > (last_LED_time + 750)) {  //every 3/4 second
            sequence = sequence + 1;
            if (sequence == 2) {
                //return to default Programming... showing battery voltages.
                start_8_seg_sequence(NORMAL_VOLT_READ);
            }
            last_LED_time = millis();
        }
    } else if (segment_8_run == NEW_CALIB) {
        if (triple_calibration) {
            switch (sequence) {
                case 0:
                    disp->writeString("CAL");
                    break;
                case 1:
                    disp->writeString("DONE");
                    break;
                case 2:
                    disp->writeString("CAL1");
                    break;
                case 3:
                    disp->writeFloat(calib[0]);
                    break;
                case 4:
                    disp->writeString("CAL2");
                    break;
                case 5:
                    disp->writeFloat(calib[1]);
                    break;
                case 6:
                    disp->writeString("CAL3");
                    break;
                case 7:
                    disp->writeFloat(calib[2]);
                    break;
            }

            if (millis() > (last_LED_time + 750)) {  //every 3/4 second
                sequence++;
                if (sequence == 8) {
                    start_8_seg_sequence(NORMAL_VOLT_READ);  //return to default Programming... showing battery voltages.
                }
                last_LED_time = millis();
            }
        } else {
            switch (sequence) {
                case 0:
                    disp->writeString("CAL");
                    break;
                case 1:
                    disp->writeString("DONE");
                    break;
                case 2:
                    disp->writeString("CAL");
                    break;
                case 3:
                    disp->writeFloat(vref_guess);
                    break;
            }

            if (millis() > (last_LED_time + 750)) {  //every 3/4 second
                sequence++;
                if (sequence == 4) {
                    start_8_seg_sequence(NORMAL_VOLT_READ);  //return to default Programming... showing battery voltages.
                }
                last_LED_time = millis();
            }
        }
    }
}

void PDB::start_8_seg_sequence(SEQ_NUM sequence_num) {
    segment_8_run = sequence_num;  //rel the display to run the right sequence
    sequence = 0;                  //and reset the step in the sequence to zero.
    last_LED_time = millis();
}

void PDB::setup_sensing() {
    //pinMode(ENABLE, OUTPUT);
    pinMode(CALIB_BUTTON, INPUT);

    //for stable (if unknown) internal voltage.
    analogReference(INTERNAL);

    //let's turn enable on, so we can measure the battery voltage.
    //digitalWrite(ENABLE, HIGH);

    if (triple_calibration) {
        update_triple_calibration();  //and then update the calibration values I use based on what I now know
    } else {
        float val = get_calibration();
        if (val > 0) {  //by convention, val is -1 if there's no calibration.
            vref_guess = val;
        }
    }
}

//measures the battery cells. Should call at least twice a second, but preferrably 5x a second.  No use faster.
void PDB::measure_cells() {
    int r_cell1 = analogRead(CELL1);
    int r_cell2 = analogRead(CELL2);
    int r_cell3 = analogRead(CELL3);

    // v_cell1 = float(r_cell1) * (R2 + R5) / R5 * calib[0] / ADC_COUNTS;
    // v_cell2 = float(r_cell2) * (R3 + R6) / R6 * calib[1] / ADC_COUNTS;
    // v_cell3 = float(r_cell3) * (R4 + R7) / R7 * calib[2] / ADC_COUNTS;

    v_cell1 = float(r_cell1) * 4.0 / float(ADC_COUNTS) * 5.0;
    v_cell2 = (float(r_cell2) * 4.0 / float(ADC_COUNTS) * 5.0) - v_cell1;
    v_cell3 = (float(r_cell3) * 4.0 / float(ADC_COUNTS) * 5.0) - v_cell2 - v_cell1;
    
    // dv_cell2 = v_cell2 - v_cell1;
    // dv_cell3 = v_cell3 - v_cell2;

    v_batt = v_cell1 + v_cell2 + v_cell3;
}

//called very frequently by loop.
uint8_t PDB::handle_calibration() {
    if (digitalRead(CALIB_BUTTON)) {  //I pressed the button, start calibrating.
        float calib_0 = calib[0];
        float calib_1 = calib[1];
        float calib_2 = calib[2];

        //i'm in triple calibration mode, but the calibration array is exactly the same as the datasheet values... which means i haven't been calibrated.
        if (triple_calibration && calib_0 == ADC_REF_NOM && calib_1 == ADC_REF_NOM & calib_2 == ADC_REF_NOM) {
            float vref_new = calibrate();
            calib_0 = calib[0];
            calib_1 = calib[1];
            calib_2 = calib[2];
            write_triple_eeprom(calib_0, calib_1, calib_2);
            return NEW_CALIB;
        } else if (!triple_calibration && get_calibration() == -1.0) {  //i'm not in triple calibration mode, and i haven't been calibrated.
            float vref_new = calibrate();
            write_single_eeprom(vref_new);
            return NEW_CALIB;
        } else {  //I have already been calibrated with something.  reset it.
            //wait untill the button press goes away.
            //This is to maintain commonality with how calibrate() works.
            delay(1);
            //reset all my calibration values as well, so my calibration values that i'm using are in lockstep with the EEPROM.
            vref_guess = ADC_REF_NOM;
            calib[0] = ADC_REF_NOM;
            calib[1] = ADC_REF_NOM;
            calib[2] = ADC_REF_NOM;

            clear_eeprom();
            return CLEAR_CALIB;
        }
    }
}

/**
 * calibrates the device.
 * assumes that a 5V line has been put to each battery cell.
 * it then calculates its best guess for vref.
 */
float PDB::calibrate() {
    //wait untill the button press goes away.
    //This is jank debouncing.
    //Have to calibrate AFTER the button is pressed, not while.  Yes, it's wierd.  PCB issue.
    while (digitalRead(CALIB_BUTTON)) {
        delay(1);
    }
    delay(100);

    //ok.  let's assume (for now) that every line has 5V put at it.
    //I want to calculate the best-fit for the 1 vref.

    //expected voltages, after the resistive dividers.
    // float expected_vc1 = EXT_CALIB_VOLT * R5 / (R2 + R5);
    // float expected_vc2 = EXT_CALIB_VOLT * R6 / (R3 + R6);
    // float expected_vc3 = EXT_CALIB_VOLT * R7 / (R4 + R7);

    int r_cell1 = analogRead(CELL1);
    int r_cell2 = analogRead(CELL2);
    int r_cell3 = analogRead(CELL3);

    //as per whiteboard math.
    // float vref1 = ADC_COUNTS / (float(r_cell1)) * R5 / (R2 + R5) * EXT_CALIB_VOLT;
    // float vref2 = ADC_COUNTS / (float(r_cell2)) * R6 / (R3 + R6) * EXT_CALIB_VOLT;
    // float vref3 = ADC_COUNTS / (float(r_cell3)) * R7 / (R4 + R7) * EXT_CALIB_VOLT;

    //calibration array values are updated.  These are only used if triple calibration is in use.
    // calib[0] = vref1;
    // calib[1] = vref2;
    // calib[2] = vref3;
    //avg all vrefs together to get best guess value
    // vref_guess = (vref1 + vref2 + vref3) / 3.0f;
    vref_guess = 1;
    return vref_guess;
}

//returns if i'm safe or not based on the most recent reading.
//Currently does only over and under voltage protection.  No time averaging.  So will need to be fancier later.
uint8_t PDB::compute_safety() {
    uint8_t unsafe;
    // Case 0: Cell voltages are below minimum values.
    if ((v_cell1 < min_cell) || (dv_cell2 < min_cell) || (dv_cell3 < min_cell)) {
        unsafe = TRUE;
        under_volt = TRUE;
    }
    // Case 1: Cell voltages are above maximum values.
    else if ((v_cell1 > max_cell) || (dv_cell2 > max_cell) || (dv_cell3 > max_cell)) {
        unsafe = TRUE;
    }
    // Case 2: Imbalanced cell voltages.
    else if ((abs(v_cell1 - dv_cell2) > d_cell) || (abs(dv_cell2 - dv_cell3) > d_cell) || (abs(dv_cell3 - v_cell1) > d_cell)) {
        imbalance = TRUE;
        unsafe = TRUE;
    }
    // Case 3: Previously undervolted, now exceeding exit threshold.  This has the effect of
    // rejecting momentary dips under the undervolt threshold but continuing to be unsafe if hovering close.
    else if (under_volt && (v_cell1 > end_undervolt) && (dv_cell2 > end_undervolt) && (dv_cell3 > end_undervolt)) {
        unsafe = FALSE;
        under_volt = FALSE;
    }
    // Case 4: Imbalanced, previously undervolted, and now exceeding exit thresholds.
    else if (imbalance && (abs(v_cell1 - dv_cell2) < end_d_cell) && (abs(dv_cell2 - dv_cell3) < end_d_cell) && (abs(dv_cell3 - v_cell1) < end_d_cell)) {
        unsafe = FALSE;
        imbalance = FALSE;
    }
    // Case 5: All is well.
    else {
        unsafe = FALSE;
    }
    return unsafe;
}

void PDB::buzz(uint8_t should_buzz) {
    if (should_buzz) {
        tone(BUZZER, NOTE_A5);
    } else {
        noTone(BUZZER);
    }
}

void PDB::handle_safety() {
    unsafe_status = compute_safety();  //Currently, just does basic sensing.  Should get updated.
    buzz(unsafe_status);
}

uint8_t PDB::is_unsafe() {
    return unsafe_status;
}

float PDB::get_calibration() {
    if (EEPROM.read(0) == 'C' && EEPROM.read(1) == 'A' && EEPROM.read(2) == 'L' && EEPROM.read(3) == ':') {
        //instantiate a float.
        float f = 0.0f;
        EEPROM.get(4, f);  //calibration value is stored in location 4
        return f;
    } else {
        return -1.0;
    }
}

//updates the global "calib" array based on EEPROM.
//Calib will contain datasheet values (2.56) if there's no calibration.
//If there is, the appopriate elements will be updated.
void PDB::update_triple_calibration() {
    int next_addr = 0;
    for (int i = 1; i <= 3; i++) {
        char first = EEPROM.read(next_addr);
        char second = EEPROM.read(next_addr + 1);
        char third = EEPROM.read(next_addr + 2);
        char fourth = EEPROM.read(next_addr + 3);
        char fifth = EEPROM.read(next_addr + 4);

        // Only takes in a valid calibration -  no chance of using partial triple calibration left over in single mode.
        if (first == 'C' && second == 'A' && third == 'L' && fourth == char(i + '0') && fifth == ':') {
            float f = 0.0f;                //instantiate a float.
            EEPROM.get(next_addr + 5, f);  //calibration value is stored in location 4
            calib[i - 1] = f;              // put value into the calibration array
            next_addr = next_addr + 5 + sizeof(float);
        } else {
            return;
        }
    }
}

void PDB::write_single_eeprom(float val) {
    //Write one-value calibration to eeprom in the following format.
    //This code may not be used in three-calibration mode, which will be standard.
    EEPROM.write(0, 'C');
    EEPROM.write(1, 'A');
    EEPROM.write(2, 'L');
    EEPROM.write(3, ':');

    //Needs arduino 1.6.12 or later.
    EEPROM.put(4, val);
}

void PDB::write_triple_eeprom(float val1, float val2, float val3) {
    // Write one-value calibration to eeprom in the following format:
    // "CAL1:<float1>CAL2:<float2>CAL3<float3>

    int next_addr = 0;
    for (int i = 1; i <= 3; i++) {
        EEPROM.write(next_addr, 'C');
        next_addr++;
        EEPROM.write(next_addr, 'A');
        next_addr++;
        EEPROM.write(next_addr, 'L');
        next_addr++;
        EEPROM.write(next_addr, char(i + '0'));
        next_addr++;
        EEPROM.write(next_addr, ':');
        next_addr++;
        if (i == 1) {
            EEPROM.put(next_addr, val1);
        } else if (i == 2) {
            EEPROM.put(next_addr, val2);
        } else if (i == 3) {
            EEPROM.put(next_addr, val3);
        }
        next_addr = next_addr + sizeof(float);
    }
}

void PDB::clear_eeprom() {
    // Writes 0 into all positions of the eeprom.
    for (int i = 0; i < EEPROM.length(); i++) {
        EEPROM.write(i, 0);
    }
}

/* *************** NEW PDB HELPER FUNCTIONS *************** */

char PDB::convert(char number) { //converts input char to another char
  if (isdigit(number)) {
    String alphabet = "abcdefghijklmnopqrstuvwxyz";
    return alphabet.charAt((int) (number - '0'));
  } else {
    return number;
  }
}

String PDB::removeChar(String phrase, char remove) {
  String newPhrase = "    ";
  int k = 0;
  for (int i = 0; i < phrase.length(); i++) {
    if (phrase.charAt(i) != remove) {
      newPhrase[k] = phrase[i];
      k += 1;
    }
    if (k == 4) {
      break;
    }
  }
  return newPhrase;
}