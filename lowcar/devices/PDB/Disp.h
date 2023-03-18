#ifndef DISP_H
#define DISP_H
#include <Wire.h>
#include "pdb_defs.h"

class Disp {

    public:
        Disp();
        void clearDisp();
        void write(String phrase, int length);
        int printDigit(char Chara, int digitPort);
        void expanderWrite(byte data);
        String DisplayChars(String Phrase);
        char convert(char number);
        String removeChar(String phrase, char remove);

    private:
        // Byte List for Seven Segment Characters
        const static int charsInArray = 27;
        const static byte INVERTED_7SEG_CHARS[charsInArray][2] { 
            {0b11101011,'0'},//0
            {0b11101111,'a'},//0.         
            {0b01100000,'1'},//1
            {0b01100100,'b'},//1.
            {0b01011011,'2'},//2
            {0b01011111,'c'},//2.
            {0b01111010,'3'},//3
            {0b01111110,'d'},//3.
            {0b11110000,'4'},//4
            {0b11110100,'e'},//4.
            {0b10111010,'5'},//5
            {0b10111110,'f'},//5.
            {0b10111011,'6'},//6
            {0b10111111,'g'},//6.
            {0b11101000,'7'},//7
            {0b11101100,'h'},//7.
            {0b11111011,'8'},//8
            {0b11111111,'i'},//8.
            {0b11111010,'9'},//9
            {0b11111110,'j'},//9

            {0b11111001,'A'},//A
            {0b10011001,'F'},//F
            {0b10000011,'L'},//L
            {0b10011011,'E'},
            {0b10001011,'C'},//C           
            {0b11100011,'U'},//U
            {0b00000000,' '}//blank
            };
};

#endif