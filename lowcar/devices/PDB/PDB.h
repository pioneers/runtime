#ifndef PDB_H
#define PDB_H

#include <Wire.h>
#include <stdint.h>
#include "Device.h"
#include "pdb_defs.h"
#include "pitches.h"

const int numOfDigits = 4;

class PDB : public Device {
  public:
    // Constructor
    PDB();

    // overriden functions from Device class; see descriptions in Device.h
    virtual size_t device_read(uint8_t param, uint8_t* data_buf);
    virtual void device_actions();
    void expanderWrite(byte data);
    void writeString(char* str);
    void clearDisp();
    void writeFloat(float val);

  private:
    /* VoltageTracker component */

    float v_cell1;     // param 3
    float v_cell2;     // param 4
    float v_cell3;     // param 5
    float v_batt;      // param 6
    float vref_guess;  // param 9

    /* disp_8 component */

    int digitPins[numOfDigits];
    unsigned long last_seg_change;  // Time the last LED switched
    unsigned long last_measure_time;
    int sequence;  // used to switch states for the display.  Remember that the handle_8_segment cannot be blocking.

    void measure_cells();
    void handle_8_segment();

    const static uint8_t INVERTED_7SEG_CHARS[][2];
};

#endif
