#ifndef DISP_H
#define DISP_H
#include <Wire.h>
#include "pdb_defs.h"

class Disp {

    public:
        Disp();
        void clearDisp();
        void expanderWrite(byte data);
        void writeFloat(float val);
        void writeString(char* str);
    private:
        // Byte List for Seven Segment Characters
        const static uint8_t INVERTED_7SEG_CHARS[][2];
};

#endif