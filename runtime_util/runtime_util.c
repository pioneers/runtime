#include "runtime_util.h"

/*
 * Definition of each lowcar device
 * Note: "int" type is int32_t to be consistent on Raspberry Pi and Arduino
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

device_t DummyDevice = {
    .type = 14,
    .name = "DummyDevice",
    .num_params = 16,
    .params = {
        // Read-only
        {.name = "RUNTIME",     .type = "int"   , .read = 1, .write = 0},
        {.name = "SHEPHERD",    .type = "float" , .read = 1, .write = 0},
        {.name = "DAWN",        .type = "bool"  , .read = 1, .write = 0},
        {.name = "DEVOPS",      .type = "int"   , .read = 1, .write = 0},
        {.name = "ATLAS",       .type = "float" , .read = 1, .write = 0},
        {.name = "INFRA",       .type = "bool"  , .read = 1, .write = 0},
        // Write-only
        {.name = "SENS",        .type = "int"   , .read = 0, .write = 1},
        {.name = "PDB",         .type = "float" , .read = 0, .write = 1},
        {.name = "MECH",        .type = "bool"  , .read = 0, .write = 1},
        {.name = "CPR",         .type = "int"   , .read = 0, .write = 1},
        {.name = "EDU",         .type = "float" , .read = 0, .write = 1},
        {.name = "EXEC",        .type = "bool"  , .read = 0, .write = 1},
        // Read-able and write-able
        {.name = "PIEF",        .type = "int"   , .read = 1, .write = 1},
        {.name = "FUNTIME",     .type = "float" , .read = 1, .write = 1},
        {.name = "SHEEP",       .type = "bool"  , .read = 1, .write = 1},
        {.name = "DUSK",        .type = "int"   , .read = 1, .write = 1},
    }
}


/* An array that holds pointers to the structs of each lowcar device */
device_t* DEVICES[DEVICES_LENGTH] = {&LimitSwitch, &LineFollower, &Potentiometer, NULL, &BatteryBuzzer,
                                     &TeamFlag, NULL, &ServoControl, NULL, NULL,
                                     &YogiBear, &RFID, &PolarBear, &KoalaBear, &DummyDevice};


device_t* get_device(uint16_t device_type) {
    if (0 <= device_type && device_type < DEVICES_LENGTH) {
       return DEVICES[device_type];
    }
    return NULL;
}


uint16_t device_name_to_type(char* dev_name) {
    for (int i = 0; i < DEVICES_LENGTH; i++) {
        if (DEVICES[i] != NULL && strcmp(DEVICES[i]->name, dev_name) == 0) {
            return i;
        }
    }
    return -1;
}

char* get_device_name(uint16_t device_type) {
    device_t* device = get_device(device_type);
    return device->name;
}

param_desc_t* get_param_desc(uint16_t dev_type, char* param_name) {
    device_t* device = get_device(dev_type);
    if (device == NULL) {
        return NULL;
    }
    for (int i = 0; i < device->num_params; i++) {
        if (strcmp(param_name, device->params[i].name) == 0) {
            return &device->params[i];
        }
    }
    return NULL;
}


int8_t get_param_idx(uint16_t dev_type, char* param_name) {
    device_t* device = get_device(dev_type);
    if (device == NULL) {
        return -1;
    }
	  for (int i = 0; i < device->num_params; i++) {
        if (strcmp(param_name, device->params[i].name) == 0) {
            return i;
        }
    }
    return -1;
}


char* BUTTON_NAMES[] = {
    "button_a", "button_b", "button_x", "button_y", "l_bumper", "r_bumper", "l_trigger", "r_trigger",
    "button_back", "button_start", "l_stick", "r_stick", "dpad_up", "dpad_down", "dpad_left", "dpad_right", "button_xbox"
};
char* JOYSTICK_NAMES[] = {
    "joystick_left_x", "joystick_left_y", "joystick_right_x", "joystick_right_y"
};

char** get_button_names() {
    return BUTTON_NAMES;
}

char** get_joystick_names() {
    return JOYSTICK_NAMES;
}
