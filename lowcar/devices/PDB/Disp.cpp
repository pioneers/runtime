// versions of commands based on SevenSeg library commands changed for Runtime specifically
#include "Disp.h"
#include "pdb_defs.h"
#define expander B0100000 //hardwired ID of the expander
//disp.write()
//disp.clearDisp()
const uint8_t Disp::INVERTED_7SEG_CHARS[][2] = { 
    {0b11101011,'0'},//0         
    {0b01100000,'1'},//1
    {0b01011011,'2'},//2
    {0b01111010,'3'},//3
    {0b11110000,'4'},//4
    {0b10111010,'5'},//5
    {0b10111011,'6'},//6
    {0b11101000,'7'},//7
    {0b11111011,'8'},//8
    {0b11111010,'9'},//9

    {0b11111001,'A'},//A
    {0b10011001,'F'},//F
    {0b10000011,'L'},//L
    {0b10011011,'E'},
    {0b10001011,'C'},//C           
    {0b11100011,'U'},//U
    {0b00000000,' '}//blank
  };
Disp::Disp() {
    this->digitPins[0] = DISP_PIN_1;
    this->digitPins[1] = DISP_PIN_2;
    this->digitPins[2] = DISP_PIN_3;
    this->digitPins[3] = DISP_PIN_4;
}

void Disp::clearDisp() { //reset all the pins so none of the digits are displayed
  digitalWrite(DISP_PIN_1, LOW);//Set all back to LOW after 
  digitalWrite(DISP_PIN_2, LOW);
  digitalWrite(DISP_PIN_3, LOW);
  digitalWrite(DISP_PIN_4, LOW);
  expanderWrite(0b11111111);
}

void Disp::expanderWrite(byte data) { //write the given byte data to the display //this DISPLAYS
  Wire.beginTransmission(expander); 
  Wire.write(data);
  Wire.endTransmission(); 
}

void Disp::writeString(char* str) {
  uint8_t bytearray[strlen(str)];
  int index = 0;
  for (int i = 0; i < strlen(str); i++) {
    for (int j = 0; j < 17; j++) {
      if(Disp::INVERTED_7SEG_CHARS[j][1] == str[i]) {
        uint8_t justChar = Disp::INVERTED_7SEG_CHARS[j][0];
        if(i == strlen(str)-1) {
          bytearray[index] = justChar;
          index++;
        }
        else if(str[i+1] == '.') {
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
  //debugging notes: changed byte array size to strlen(str)
  for(int k = 0; k < strlen(str) && k < 4; k++) {
    digitalWrite(digitPins[k], HIGH);
    expanderWrite(~bytearray[k]);
    delay(250);
    clearDisp(); // digitalWrite(digitPins[k], LOW);
  }
}

void Disp::writeFloat(float val) {
	char str[6];
	int integer_part = (int) val;
	int decimal_part = (int) ((val - integer_part) * 100);

	sprintf(str, "%d.%d", integer_part, decimal_part);
	writeString(str);
}
