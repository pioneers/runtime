/**
 * Dummy lowcar device to test DEV_HANDLER
 */
#include "message.h"
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

int incomingByte = 0; // for incoming serial data

void setup() {
  Serial.begin(19200); // opens serial port, sets data rate to 9600 bps
}

// A Ping message is 0x00 42 10 11
void loop() {
  // reply only when you receive data:
  while (Serial.available()) {
    // read the incoming byte:
    byte iByte = Serial.read();
    byte inData[32];    // Byte array to fill with the incoming message
    byte index;         // Next empty index of the byte array

    // Determine whether the incoming byte is the start of a new message or part of an existing message
    if (iByte = 0x00) {
        Serial.println("start of packet");
        index = 0;
    } else {
        Serial.println("adding to msg");
        index += 1;
    }
    inData[index] = iByte;

    // 0x11 is the last byte of a Ping message. Hardcoding this at the moment
    if(iByte = 0x11){
      Serial.println("end of msg");
      byte msg[10];
      msg[0] = 0x00;
      msg[1] = 0x12;
      msg[2] = 0x34;
      msg[3] = 0x56;
      msg[4] = 0x78;
      msg[5] = 0x90;
      msg[6] = 0x11;
      msg[7] = 0x12;
      msg[8] = 0x13;
      msg[9] = 0x14;
      Serial.write(msg, 10);
    }
  }
}
