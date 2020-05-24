/* Defines devices.json in C to avoid reading from JSON file everytime
A Device is defined by a 16-bit type, an 8-bit year, and a 64-bit uid*
(There's a 1-to-1 mapping between types and device names)
See hibikeDevices.json for the numbers. */

/* The number of functional devices */
int NUM_DEVICES = 11;
/* The largest device type number + 1
 * DEVICES_LENGTH != NUM_DEVICES because some are NULL (Ex: type 6, 8, 9)
*/
int DEVICES_LENGTH = 14;

// A struct defining the params of a device
typedef struct param_desc {
  char* name;
  char* type;
  int read;
  int write;
} param_desc_t;

// A struct defining a dev
typedef struct dev {
  int type;
  char* name;
  int num_params;
  param params[32]; // There are up to 32 possible parameters for a device
} dev_t;

dev_t LimitSwitch = {
  .type = 0,
  .name = "LimitSwitch",
  .num_params = 3,
  .params = {
    {.name = "switch0"    , .type = "int"    , .read = 1 , .write = 0 },
    {.name = "switch1"    , .type = "int"    , .read = 1 , .write = 0 },
    {.name = "switch2"    , .type = "int"    , .read = 1 , .write = 0 }
  }
};

dev_t LineFollower = {
  .type = 1,
  .name = "LineFollower",
  .num_params = 3,
  .params = {
    {.name = "left"       , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "center"     , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "right"      , .type = "float"   , .read = 1 , .write = 0 }
  }
};

 dev_t Potentiometer = {
  .type = 2,
  .name = "Potentiometer",
  .num_params = 3,
  .params = {
    {.name = "pot0"       , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "pot1"       , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "pot2"       , .type = "float"   , .read = 1 , .write = 0 }
  }
};

dev_t Encoder = {
  .type = 3,
  .name = "Encoder",
  .num_params = 1,
  .params = {
    {.name = "rotation"   , .type = "int16_t" , .read = 1 , .write = 0 }
  }
};

dev_t BatteryBuzzer = {
  .type = 4,
  .name = "BatteryBuzzer",
  .num_params = 8,
  .params = {
    {.name = "is_unsafe"  , .type = "int"     , .read = 1 , .write = 0 },
    {.name = "calibrated" , .type = "int"     , .read = 1 , .write = 0  },
    {.name = "v_cell1"    , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "v_cell2"    , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "v_cell3"    , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "v_batt"     , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "dv_cell2"   , .type = "float"   , .read = 1 , .write = 0 },
    {.name = "dv_cell3"   , .type = "float"   , .read = 1 , .write = 0 }
  }
};

 dev_t TeamFlag = {
  .type = 5,
  .name = "TeamFlag",
  .num_params = 7,
  .params = {
    {.name = "mode"  , .type = "int"    , .read = 1 , .write = 1},
    {.name = "blue"  , .type = "int"    , .read = 1 , .write = 1},
    {.name = "yellow", .type = "int"    , .read = 1 , .write = 1},
    {.name = "led1"  , .type = "int"    , .read = 1 , .write = 1},
    {.name = "led2"  , .type = "int"    , .read = 1 , .write = 1},
    {.name = "led3"  , .type = "int"    , .read = 1 , .write = 1},
    {.name = "led4"  , .type = "int"    , .read = 1 , .write = 1}
  }
};

dev_t ServoControl = {
  .type = 7,
  .name= "ServoControl",
  .num_params = 2,
  .params = {
    {.name = "servo0"     , .type = "float" , .read = 1 , .write = 1  },
    {.name = "servo1"     , .type = "float" , .read = 1 , .write = 1  }
  }
};

dev_t KoalaBear = {
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
      {.name = "motor_enabled_a"		, .type = "int"	, .read = 1 , .write = 1  },
      {.name = "drive_mode_a"		, .type = "int32_t"		, .read = 1 , .write = 1  },
      {.name = "pid_kp_a"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "pid_kp_b"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "pid_ki_b"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "pid_kd_b"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "enc_b"				, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "deadband_b"			, .type = "float"	, .read = 1 , .write = 1  },
      {.name = "motor_enabled_b"	, .type = "int"	, .read = 1 , .write = 1  },
      {.name = "drive_mode_b"		, .type = "int32_t"		, .read = 1 , .write = 1  }
  }
};

dev_t PolarBear = {
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
    {.name = "enc_pos"            , .type = "float"    , .read = 1 , .write = 1  },
    {.name = "enc_vel"            , .type = "float"    , .read = 1 , .write = 0  },
    {.name = "motor_current"      , .type = "float"    , .read = 1 , .write = 0  },
    {.name = "deadband"           , .type = "float"    , .read = 1 , .write = 1  }
  }
};


dev_t YogiBear = {
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
    {.name = "enc_pos"            , .type = "float"    , .read = 1 , .write = 1  },
    {.name = "enc_vel"            , .type = "float"    , .read = 1 , .write = 0  },
    {.name = "motor_current"      , .type = "float"    , .read = 1 , .write = 0  },
    {.name = "deadband"           , .type = "float"    , .read = 1 , .write = 1  }
  }
};

dev_t RFID = {
  .type = 11,
  .name = "RFID",
  .num_params = 2,
  .params = {
    {.name = "type"           , .type = "uint32_t"    , .read = 1 , .write = 0  },
    {.name = "tag_detect"     , .type = "uint8_t"     , .read = 1 , .write = 0  }
  }
};

dev_t ExampleDevice = {
  .type = 65535,
  .name = "ExampleDevice",
  .num_params = 16,
  .params = {
    {.name = "kumiko"     , .type = "int"    , .read = 1 , .write = 1  },
    {.name = "hazuki"     , .type = "uint8_t" , .read = 1 , .write = 1  },
    {.name = "sapphire"   , .type = "int8_t"  , .read = 1 , .write = 1  },
    {.name = "reina"      , .type = "uint16_t", .read = 1 , .write = 1  },
    {.name = "asuka"      , .type = "int16_t" , .read = 1 , .write = 1  },
    {.name = "haruka"     , .type = "uint32_t", .read = 1 , .write = 1  },
    {.name = "kaori"      , .type = "int32_t" , .read = 1 , .write = 1  },
    {.name = "natsuki"    , .type = "uint64_t", .read = 1 , .write = 1  },
    {.name = "yuko"       , .type = "int64_t" , .read = 1 , .write = 1  },
    {.name = "mizore"     , .type = "float"   , .read = 1 , .write = 1  },
    {.name = "nozomi"     , .type = "float"  , .read = 1 , .write = 1  },
    {.name = "shuichi"    , .type = "uint8_t" , .read = 1 , .write = 0 },
    {.name = "takuya"     , .type = "uint16_t", .read = 0, .write = 1  },
    {.name = "riko"       , .type = "uint32_t", .read = 1 , .write = 0 },
    {.name = "aoi"        , .type = "uint64_t", .read = 0, .write = 1  },
    {.name = "noboru"     , .type = "float"   , .read = 1 , .write = 0 }
  }
};
