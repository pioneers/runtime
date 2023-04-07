#ifndef PDB_DEFS
#define PDB_DEFS
#include "defs.h"

#define expander B0100000 //hardwired ID of the expander

#define CELL1 Analog::IO3
#define CELL2 Analog::IO2
#define CELL3 Analog::IO1

#define BUZZER 10
#define NET_SWITCH_PIN 16
#define ADC_COUNTS 1024

#define TX 0
#define RX 1

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

#endif
