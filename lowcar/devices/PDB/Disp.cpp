// versions of commands based on SevenSeg library commands changed for Runtime specifically
#include "Disp.h"
#include "pdb_defs.h"
#define expander B0100000 //hardwired ID of the expander
//disp.write()
//disp.clearDisp()
Disp::Disp() {

}

void Disp::clearDisp() { //reset all the pins so none of the digits are displayed
  digitalWrite(DISP_PIN_1, LOW); 
  digitalWrite(DISP_PIN_2, LOW);
  digitalWrite(DISP_PIN_3, LOW);
  digitalWrite(DISP_PIN_4, LOW);
  expanderWrite(0b11111111);
}

void Disp::write(String phrase) {


    
  // for (int i = 0; i < length/8; i++) {

/*  This code controls what is output when the network switch is flipped.
    This will poll approximately once every 5 ms. */



    // if (digitalRead(TOGGLE) != network_switch) {
    //   network_switch = digitalRead(TOGGLE);
    //   Serial1.println(digitalRead(TOGGLE));
    // }

    //Looped in Device.cpp loop() function through device_actions()
    printDigit(phrase[0], DISP_PIN_1);
    delay(1);
    printDigit(phrase[1], DISP_PIN_2);
    delay(1);
    printDigit(phrase[2], DISP_PIN_3);
    delay(1);
    printDigit(phrase[3], DISP_PIN_4);
    delay(1);
  // }
}
// loop through the array of characters in PDB.h to identify character that matches Chara
int Disp::printDigit(char Chara, int digitPort) { 
  clearDisp();
  int character = -1;
  digitalWrite(digitPort, HIGH); //Arduino function 
  for(int i = 0 ; i < Disp::charsInArray ; i++) {
    if (Chara == Char[i][1]) {
     character = i;
    }
  }
  expanderWrite(~Char[character][0]);
}

void Disp::expanderWrite(byte data) { //write the given byte data to the display
  Wire.beginTransmission(expander);
  Wire.write(data);
  Wire.endTransmission(); 
}

String Disp::DisplayChars(String Phrase) { //no scrolling
  int stringLength = Phrase.length();
  int delay_factor = 4 * 2;
  int decimal_pos = -1;
  String toPrint = "    ";

  for (int i = 0; i < 4 + 1; i++) {
    if (Phrase.charAt(i) == '.') { //only supports 1 decimal point
      stringLength -= 1;
      decimal_pos = i;
      break;
    }
  }
  Phrase = removeChar(Phrase, '.');

  for(int i = 0; i < 4; i++) {
    if (i+1 <= stringLength && decimal_pos == i+1) {
      toPrint[i] = convert(Phrase[i]);
    } else if (i+1 <= stringLength) {
      toPrint[i] = Phrase[i];
    }
  }
  return toPrint;
}

char convert(char number) { //converts input char to another char
  if (isdigit(number)) {
    String alphabet = "abcdefghijklmnopqrstuvwxyz";
    return alphabet.charAt((int) (number - '0'));
  } else {
    return number;
  }
}

String removeChar(String phrase, char remove) {
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