#include "devices.h"

/*
 * Definition of each lowcar device
*/
device_t LimitSwitch = {
  .type = 0,
  .name = "LimitSwitch",
  .num_params = 3,
  .params = {
    {.name = "switch0"    , .type = "bool"    , .read = 1 , .write = 0 },
    {.name = "switch1"    , .type = "bool"    , .read = 1 , .write = 0 },
    {.name = "switch2"    , .type = "bool"    , .read = 1 , .write = 0 }
  }
};

device_t LineFollower = {
  .type = 1,
  .name = "LineFollower",
  .num_params = 3,
  .params = {
    {.name = "left"       , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "center"     , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "right"      , .type = "float"   , .read = 1 , .write = 0 }
  }
};

device_t Potentiometer = {
  .type = 2,
  .name = "Potentiometer",
  .num_params = 3,
  .params = {
    {.name = "pot0"       , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "pot1"       , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "pot2"       , .type = "float"   , .read = 1 , .write = 0 }
  }
};

device_t Encoder = {
  .type = 3,
  .name = "Encoder",
  .num_params = 1,
  .params = {
    {.name = "rotation"   , .type = "int" , .read = 1 , .write = 0 }
  }
};

device_t BatteryBuzzer = {
  .type = 4,
  .name = "BatteryBuzzer",
  .num_params = 8,
  .params = {
    {.name = "is_unsafe"  , .type = "bool"     , .read = 1 , .write = 0 },
    {.name = "calibrated" , .type = "bool"     , .read = 1 , .write = 0  },
    {.name = "v_cell1"    , .type = "float"    , .read = 1 , .write = 0 },
    {.name = "v_cell2"    , .type = "float"    , .read = 1 , .write = 0 },
    {.name = "v_cell3"    , .type = "float"    , .read = 1 , .write = 0 },
    {.name = "v_batt"     , .type = "float"    , .read = 1 , .write = 0 },
    {.name = "dv_cell2"   , .type = "float"    , .read = 1 , .write = 0 },
    {.name = "dv_cell3"   , .type = "float"    , .read = 1 , .write = 0 }
  }
};

device_t TeamFlag = {
  .type = 5,
  .name = "TeamFlag",
  .num_params = 7,
  .params = {
    {.name = "mode"  , .type = "bool"    , .read = 1 , .write = 1},
    {.name = "blue"  , .type = "bool"    , .read = 1 , .write = 1},
    {.name = "yellow", .type = "bool"    , .read = 1 , .write = 1},
    {.name = "led1"  , .type = "bool"    , .read = 1 , .write = 1},
    {.name = "led2"  , .type = "bool"    , .read = 1 , .write = 1},
    {.name = "led3"  , .type = "bool"    , .read = 1 , .write = 1},
    {.name = "led4"  , .type = "bool"    , .read = 1 , .write = 1}
  }
};

device_t ServoControl = {
  .type = 7,
  .name= "ServoControl",
  .num_params = 2,
  .params = {
    {.name = "servo0"     , .type = "float" , .read = 1 , .write = 1  },
    {.name = "servo1"     , .type = "float" , .read = 1 , .write = 1  }
  }
};

device_t KoalaBear = {
  .type = 13,
  .name = "KoalaBear",
  .num_params = 16,
  .params = {
      {.name = "duty_cycle_a"		, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "duty_cycle_b"		, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "pid_ki_a"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "pid_kd_a"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "enc_a"				, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "deadband_a"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "motor_enabled_a"	, .type = "bool"    , .read = 1 , .write = 1  },
      {.name = "drive_mode_a"		, .type = "int"		, .read = 1 , .write = 1  },
      {.name = "pid_kp_a"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "pid_kp_b"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "pid_ki_b"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "pid_kd_b"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "enc_b"				, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "deadband_b"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "motor_enabled_b"	, .type = "bool"	, .read = 1 , .write = 1  },
      {.name = "drive_mode_b"		, .type = "int"		, .read = 1 , .write = 1  }
  }
};

device_t PolarBear = {
  .type = 12,
  .name = "PolarBear",
  .num_params = 14,
  .params = {
    {.name = "duty_cycle"          , .type = "float"    , .read = 1 , .write = 1  },
    {.name = "pid_pos_setpoint"    , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_pos_kp"          , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_pos_ki"          , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_pos_kd"          , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_vel_setpoint"    , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_vel_kp"          , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_vel_ki"          , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_vel_kd"          , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "current_thresh"      , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "enc_pos"             , .type = "float"    , .read = 1 , .write = 1  },
    {.name = "enc_vel"             , .type = "float"    , .read = 1 , .write = 0  },
    {.name = "motor_current"       , .type = "float"    , .read = 1 , .write = 0  },
    {.name = "deadband"            , .type = "float"    , .read = 1 , .write = 1  }
  }
};

device_t YogiBear = {
  .type = 10,
  .name = "YogiBear",
  .num_params = 14,
  .params = {
    {.name = "duty_cycle"          , .type = "float"    , .read = 1 , .write = 1  },
    {.name = "pid_pos_setpoint"    , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_pos_kp"          , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_pos_ki"          , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_pos_kd"          , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_vel_setpoint"    , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_vel_kp"          , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_vel_ki"          , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "pid_vel_kd"          , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "current_thresh"      , .type = "float"    , .read = 0 , .write = 1  },
    {.name = "enc_pos"             , .type = "float"    , .read = 1 , .write = 1  },
    {.name = "enc_vel"             , .type = "float"    , .read = 1 , .write = 0  },
    {.name = "motor_current"       , .type = "float"    , .read = 1 , .write = 0  },
    {.name = "deadband"            , .type = "float"    , .read = 1 , .write = 1  }
  }
};

device_t RFID = {
  .type = 11,
  .name = "RFID",
  .num_params = 2,
  .params = {
    {.name = "id"           , .type = "int"     , .read = 1 , .write = 0  },
    {.name = "detect_tag"   , .type = "int"     , .read = 1 , .write = 0  }
  }
};

device_t ExampleDevice = {
  .type = 65535,
  .name = "ExampleDevice",
  .num_params = 16,
  .params = {
    {.name = "kumiko"     , .type = "bool"  , .read = 1 , .write = 1  },
    {.name = "hazuki"     , .type = "int"   , .read = 1 , .write = 1  },
    {.name = "sapphire"   , .type = "int"   , .read = 1 , .write = 1  },
    {.name = "reina"      , .type = "int"   , .read = 1 , .write = 1  },
    {.name = "asuka"      , .type = "int"   , .read = 1 , .write = 1  },
    {.name = "haruka"     , .type = "int"   , .read = 1 , .write = 1  },
    {.name = "kaori"      , .type = "int"   , .read = 1 , .write = 1  },
    {.name = "natsuki"    , .type = "int"   , .read = 1 , .write = 1  },
    {.name = "yuko"       , .type = "int"   , .read = 1 , .write = 1  },
    {.name = "mizore"     , .type = "float" , .read = 1 , .write = 1  },
    {.name = "nozomi"     , .type = "float" , .read = 1 , .write = 1  },
    {.name = "shuichi"    , .type = "int"   , .read = 1 , .write = 0 },
    {.name = "takuya"     , .type = "int"   , .read = 0, .write = 1  },
    {.name = "riko"       , .type = "int"   , .read = 1 , .write = 0 },
    {.name = "aoi"        , .type = "int"   , .read = 0, .write = 1  },
    {.name = "noboru"     , .type = "float" , .read = 1 , .write = 0 }
  }
};

/* An array that holds pointers to the structs of each lowcar device */
device_t* DEVICES[DEVICES_LENGTH] = {&LimitSwitch, &LineFollower, &Potentiometer, &Encoder, &BatteryBuzzer,
                                     &TeamFlag, NULL, &ServoControl, NULL, NULL,
                                     &YogiBear, &RFID, &PolarBear, &KoalaBear};

device_t* get_device(uint16_t device_type) {
    return DEVICES[device_type];
}

uint16_t device_name_to_type(char* dev_name) {
    for (int i = 0; i < DEVICES_LENGTH; i++) {
        if (DEVICES[i] != NULL && strcmp(DEVICES[i]->name, dev_name) == 0) {
            return i;
        }
    }
    return -1;
}

char* device_type_to_name(uint16_t dev_type) {
    return DEVICES[dev_type]->name;
}

void all_params_for_device_type(uint16_t dev_type, char* param_names[]) {
    int num_params = DEVICES[dev_type]->num_params;
    for (int i = 0; i < num_params; i++) {
        param_names[i] = DEVICES[dev_type]->params[i].name;
    }
}

uint8_t readable(uint16_t dev_type, char* param_name) {
    int num_params = DEVICES[dev_type]->num_params;
    for (int i = 0; i < num_params; i++) {
        if (strcmp(DEVICES[dev_type]->params[i].name, param_name) == 0) {
            return DEVICES[dev_type]->params[i].read;
        }
    }
    return -1;
}

uint8_t writeable(uint16_t dev_type, char* param_name) {
    int num_params = DEVICES[dev_type]->num_params;
    for (int i = 0; i < num_params; i++) {
        if (strcmp(DEVICES[dev_type]->params[i].name, param_name) == 0) {
            return DEVICES[dev_type]->params[i].write;
        }
    }
    return -1;
}

char* get_param_type(uint16_t dev_type, char* param_name) {
    int num_params = DEVICES[dev_type]->num_params;
    for (int i = 0; i < num_params; i++) {
        if (strcmp(DEVICES[dev_type]->params[i].name, param_name) == 0) {
            return DEVICES[dev_type]->params[i].type;
        }
    }
    return NULL;
}

uint8_t get_param_id(uint16_t dev_type, char* param_name) {
    int num_params = DEVICES[dev_type]->num_params;
	for (int i = 0; i < num_params; i++) {
        if (strcmp(param_name, DEVICES[dev_type]->params[i].name) == 0) {
            return i;
        }
    }
    return -1;
}
