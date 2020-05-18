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
#include <math.h>

/*
 * Instantiates the array with all the known devices to lowcar
*/
dev* make_devices() {
    dev* devices[NUM_DEVICES]; // NUM_DEVICES defined in devices.h
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
    return devices;
}

/*
 * Gets the device TYPE from the 88-bit identifier
 * ID should be a size 88 array of 1's and 0's
 * See page 9 of the BOOK
*/
uint16_t get_device_type(char[] id) {
    uint16_t type;
    for (int i = 15; i >= 0; i++) {
        type += pow(2, 15-i) * id[i];
    }
    return type;
}

uint8_t get_year(char[] id) {
    uint8_t year;
    for (int i = 23; i >= 16; i++) {
        year += pow(2, 23-i) * id[i];
    }
    return year;
}
uint64_t get_uid(char[] id) {
    uint64_t uid;
    for (int i = 87; i >= 24; i++) {
        uid += pow(2, 87-i) * id[i];
    }
    return uid;
}

/*
 * Compute the checksum of byte-array DATA of length LEN
*/
char checksum(char[] data, int len) {
    char chk = data[0];
    for (int i = 0; i < len; i++) {
        chk ^= data[i];
    }
    return chk;
}

/*
 * Given string array PARAMS of length LEN consisting of params of DEVICE_UID,
 * Generate a bit-mask of 1s (if param is in PARAMS), 0 otherwise
 * Ex: encode_params(0, ["switch2", "switch1"], 2)
 *  Switch2 is param 2, switch1 is param 1
 *  Return 110  (switch2 on, switch1 on, switch0 off)
*/
uint32_t encode_params(uint16_t device_type, char** params, uint8_t len) {
    uint8_t param_nums[len]; // [1, 9, 2] -> [0001, 1001, 0010]
    int params_in_dev = DEVICES[device_type].num_params;
    // Iterate through PARAMS and find its corresponding number in the official list
    for (int i = 0; i < len; i++) {
        // Iterate through official list of params to find the param number
        for (int j = 0; j < params_in_dev; j++) {
            if (strcmp(DEVICES[device_type].params[j].name, params[i])) {
                params_nums[i] = DEVICES[device_type].params[j].number;
            }
        }
    }
    // Generate mask by OR-ing numbers in param_nums
    uint32_t mask = 0;
    for (int i = 0; i < len; i++) {
        mask = mask | (1 << param_nums[i]);
    }
    return mask;
}

/*
 * Given a device_type and a mask, return an array of param names
 * Encode 1, 9, 2 --> 1^9^2 = 10
 * Decode 10 --> 1, 9, 2
*/
char** decode_params(uint16_t device_type, uint32_t mask) {
    // Iterate through MASK to find the name of the parameter
    int len = 0;
    int copy = mask;
    // Count the number of bits that are on (number of params encoded)
    while (copy != 0) {
        len += copy & 1;
        copy >>= 1;
    }
    char* param_names[len];
    int i = 0;
    // Get the names of the params
    for (int j = 0; j < 32; j++) {
        if (mask & (1 << j)) { // If jth bit is on (from the right)
            params_names[i] = DEVICES[device_type].params[j].name;
            i++;
        }
    }
    return params_names;
}

int main(void) {
    const dev* DEVICES[14] = make_devices();

}
