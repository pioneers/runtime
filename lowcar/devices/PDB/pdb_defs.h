#ifndef PDB_DEFS
#define PDB_DEFS
#include "defs.h"

//voltage_sense
//All resistor values (R2-7) are in kOhms
// #define R2 10.0
// #define R3 30.0
// #define R4 51.0
// #define R5 10.0
// #define R6 10.0
// #define R7 10.0

#define ADC_REF_NOM 2.56
#define ADC_COUNTS 1024
#define EXT_CALIB_VOLT 5

#define expander B0100000 //hardwired ID of the expander
#define BUZZER 10

#define CELL1 Analog::IO3
#define CELL2 Analog::IO2
#define CELL3 Analog::IO1

#define TOGGLE 16
#define CALIB_BUTTON 21

#define TX 0
#define RX 1

//disp_8 -- REPLACED IN PDB.cpp
/* #define A 8
#define B 9
#define C 7
#define D 2
#define E 0
#define G 15
#define H 14
#define DECIMAL_POINT 1 */

#define DISP_PIN_1 Analog::IO0
#define DISP_PIN_2 15
#define DISP_PIN_3 14
#define DISP_PIN_4 9

// Enumerated values for BatteryBuzzer and VoltageTracker
typedef enum {
    IS_UNSAFE,
    CALIBRATED,
    V_CELL1,
    V_CELL2,
    V_CELL3,
    V_BATT,
    DV_CELL2,
    DV_CELL3,
    NETWORK_SWITCH
} PARAMS;

// Enumerated values for disp_8
typedef enum {
    NORMAL_VOLT_READ,
    CLEAR_CALIB,
    NEW_CALIB
} SEQ_NUM;

#endif
