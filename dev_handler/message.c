/*
Constructs, encodes, and decodes the appropriate messages asked for by dev_handler.c
Previously hibikeMessage.py

Linux: json-c:      sudo apt-get install -y libjson-c-dev
       compile:     gcc -I/usr/include/json-c -L/usr/lib message.c -o message -ljson-c
       // Note that -ljson-c comes AFTER source files: https://stackoverflow.com/questions/31041272/undefined-reference-error-while-using-json-c
Mac:
       compile:     gcc -I/usr/local/include/json-c -L/usr/local/lib/ message.c -o message -ljson-c


*/
#include <stdio.h>
#include "message.h"

int main(void) {
    const dev* devices[14]; //NUM_DEVICES];
    devices[0] = &LimitSwitch;
    devices[1] = &LineFollower;
    devices[2] = &Potentiometer;
    devices[3] = &Encoder;
    devices[4] = &BatteryBuzzer;
    devices[5] = &TeamFlag;
    devices[6] = NULL;
    devices[7] = &ServoControl;
    devices[8] = NULL;
    devices[9] = NULL;
    devices[13] = &KoalaBear;
    devices[12] = &PolarBear;
    devices[10] = &YogiBear;
    devices[11] = &RFID;
    for (int i = 0; i < 14; i++) {
      if (devices[i]) {
        printf("Device: %d %s\n", devices[i]->type, devices[i]->name);
        printf("---Param 0: %s\n", devices[i]->params[0].name);
      }
    }
}
