/* Defines devices.json in C to avoid reading from JSON file everytime
A Device is defined by a 16-bit type, an 8-bit year, and a 64-bit uid*
(There's a 1-to-1 mapping between types and device names)
See hibikeDevices.json for the numbers. */

int NUM_DEVICES = 11; // Don't count example device

// A struct defining the params of a device
typedef struct param {
  int number;
  char* name;
  char* type;
  int read;
  int write;
} param;

// A struct defining a dev
typedef struct dev {
  int type;
  char* name;
  int num_params;
  param params[16]; // There are 16 possible parameters for a device
} dev;


const dev LimitSwitch = {
  .type = 0,
  .name = "LimitSwitch",
  .params = {
    {.number = 0  , .name = "switch0"    , .type = "int"    , .read = 1 , .write = 0 },
    {.number = 1  , .name = "switch1"    , .type = "int"    , .read = 1 , .write = 0 },
    {.number = 2  , .name = "switch2"    , .type = "int"    , .read = 1 , .write = 0 }
  }
};

const dev LineFollower = {
  .type = 1,
  .name = "LineFollower",
  .params = {
    {.number = 0  , .name = "left"       , .type = "float"   , .read = 1 , .write = 0 },
    {.number = 1  , .name = "center"     , .type = "float"   , .read = 1 , .write = 0 },
    {.number = 2  , .name = "right"      , .type = "float"   , .read = 1 , .write = 0 }
  }
};

const dev Potentiometer = {
  .type = 2,
  .name = "Potentiometer",
  .params = {
    {.number = 0  , .name = "pot0"       , .type = "float"   , .read = 1 , .write = 0 },
    {.number = 1  , .name = "pot1"       , .type = "float"   , .read = 1 , .write = 0 },
    {.number = 2  , .name = "pot2"       , .type = "float"   , .read = 1 , .write = 0 }
  }
};

const dev Encoder = {
  .type = 3,
  .name = "Encoder",
  .params = {
    {.number = 0  , .name = "rotation"   , .type = "int16_t" , .read = 1 , .write = 0 }
  }
};

const dev BatteryBuzzer = {
  .type = 4,
  .name = "BatteryBuzzer",
  .params = {
    {.number = 0  , .name = "is_unsafe"  , .type = "int"     , .read = 1 , .write = 0 },
    {.number = 1  , .name = "calibrated" , .type = "int"     , .read = 1 , .write = 0  },
    {.number = 2  , .name = "v_cell1"    , .type = "float"   , .read = 1 , .write = 0 },
    {.number = 3  , .name = "v_cell2"    , .type = "float"   , .read = 1 , .write = 0 },
    {.number = 4  , .name = "v_cell3"    , .type = "float"   , .read = 1 , .write = 0 },
    {.number = 5  , .name = "v_batt"     , .type = "float"   , .read = 1 , .write = 0 },
    {.number = 6  , .name = "dv_cell2"   , .type = "float"   , .read = 1 , .write = 0 },
    {.number = 7  , .name = "dv_cell3"   , .type = "float"   , .read = 1 , .write = 0 }
  }
};

const dev TeamFlag = {
  .type = 5,
  .name = "TeamFlag",
  .params = {
    {.number = 0  , .name = "mode"  , .type = "int"    , .read = 1 , .write = 1},
    {.number = 1  , .name = "blue"  , .type = "int"    , .read = 1 , .write = 1},
    {.number = 2  , .name = "yellow", .type = "int"    , .read = 1 , .write = 1},
    {.number = 3  , .name = "led1"  , .type = "int"    , .read = 1 , .write = 1},
    {.number = 4  , .name = "led2"  , .type = "int"    , .read = 1 , .write = 1},
    {.number = 5  , .name = "led3"  , .type = "int"    , .read = 1 , .write = 1},
    {.number = 6  , .name = "led4"  , .type = "int"    , .read = 1 , .write = 1}
  }
};

const dev ServoControl = {
  .type = 7,
  .name= "ServoControl",
  .params = {
    {.number = 0  , .name = "servo0"     , .type = "float" , .read = 1 , .write = 1  },
    {.number = 1  , .name = "servo1"     , .type = "float" , .read = 1 , .write = 1  }
  }
};

const dev KoalaBear = {
  .type = 13,
  .name = "KoalaBear",
  .params = {
      {.number = 0  , .name = "duty_cycle_a"		, .type = "float"	, .read = 1 , .write = 1  },
      {.number = 1  , .name = "duty_cycle_b"		, .type = "float"	, .read = 1 , .write = 1  },
      {.number = 2  , .name = "pid_ki_a"			, .type = "float"	, .read = 1 , .write = 1  },
      {.number = 3  , .name = "pid_kd_a"			, .type = "float"	, .read = 1 , .write = 1  },
      {.number = 4  , .name = "enc_a"				, .type = "float"	, .read = 1 , .write = 1  },
      {.number = 5  , .name = "deadband_a"			, .type = "float"	, .read = 1 , .write = 1  },
      {.number = 6  , .name = "motor_enabled_a"		, .type = "int"	, .read = 1 , .write = 1  },
      {.number = 7  , .name = "drive_mode_a"		, .type = "int32_t"		, .read = 1 , .write = 1  },
      {.number = 8  , .name = "pid_kp_a"			, .type = "float"	, .read = 1 , .write = 1  },
      {.number = 9  , .name = "pid_kp_b"			, .type = "float"	, .read = 1 , .write = 1  },
      {.number = 10  , .name = "pid_ki_b"			, .type = "float"	, .read = 1 , .write = 1  },
      {.number = 11  , .name = "pid_kd_b"			, .type = "float"	, .read = 1 , .write = 1  },
      {.number = 12  , .name = "enc_b"				, .type = "float"	, .read = 1 , .write = 1  },
      {.number = 13  , .name = "deadband_b"			, .type = "float"	, .read = 1 , .write = 1  },
      {.number = 14  , .name = "motor_enabled_b"	, .type = "int"	, .read = 1 , .write = 1  },
      {.number = 15  , .name = "drive_mode_b"		, .type = "int32_t"		, .read = 1 , .write = 1  }
  }
};

const dev PolarBear = {
  .type = 12,
  .name = "PolarBear",
  .params = {
    {.number = 0  , .name = "duty_cycle"          , .type = "float"    , .read = 1 , .write = 1  },
    {.number = 1  , .name = "pid_pos_setpoint"    , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 2  , .name = "pid_pos_kp"          , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 3  , .name = "pid_pos_ki"          , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 4  , .name = "pid_pos_kd"          , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 5  , .name = "pid_vel_setpoint"    , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 6  , .name = "pid_vel_kp"          , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 7  , .name = "pid_vel_ki"          , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 8  , .name = "pid_vel_kd"          , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 9  , .name = "current_thresh"      , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 10  , .name = "enc_pos"            , .type = "float"    , .read = 1 , .write = 1  },
    {.number = 11  , .name = "enc_vel"            , .type = "float"    , .read = 1 , .write = 0  },
    {.number = 12  , .name = "motor_current"      , .type = "float"    , .read = 1 , .write = 0  },
    {.number = 13  , .name = "deadband"           , .type = "float"    , .read = 1 , .write = 1  }
  }
};


const dev YogiBear = {
  .type = 10,
  .name = "YogiBear",
  .params = {
    {.number = 0  , .name = "duty_cycle"          , .type = "float"    , .read = 1 , .write = 1  },
    {.number = 1  , .name = "pid_pos_setpoint"    , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 2  , .name = "pid_pos_kp"          , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 3  , .name = "pid_pos_ki"          , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 4  , .name = "pid_pos_kd"          , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 5  , .name = "pid_vel_setpoint"    , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 6  , .name = "pid_vel_kp"          , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 7  , .name = "pid_vel_ki"          , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 8  , .name = "pid_vel_kd"          , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 9  , .name = "current_thresh"      , .type = "float"    , .read = 0 , .write = 1  },
    {.number = 10  , .name = "enc_pos"            , .type = "float"    , .read = 1 , .write = 1  },
    {.number = 11  , .name = "enc_vel"            , .type = "float"    , .read = 1 , .write = 0  },
    {.number = 12  , .name = "motor_current"      , .type = "float"    , .read = 1 , .write = 0  },
    {.number = 13  , .name = "deadband"           , .type = "float"    , .read = 1 , .write = 1  }
  }
};

const dev RFID = {
  .type = 11,
  .name = "RFID",
  .params = {
    {.number = 0  , .name = "type"           , .type = "uint32_t"    , .read = 1 , .write = 0  },
    {.number = 1  , .name = "tag_detect"     , .type = "uint8_t"     , .read = 1 , .write = 0  }
  }
};

const dev ExampleDevice = {
  .type = 65535,
  .name = "ExampleDevice",
  .params = {
    {.number = 0  , .name = "kumiko"     , .type = "int"    , .read = 1 , .write = 1  },
    {.number = 1  , .name = "hazuki"     , .type = "uint8_t" , .read = 1 , .write = 1  },
    {.number = 2  , .name = "sapphire"   , .type = "int8_t"  , .read = 1 , .write = 1  },
    {.number = 3  , .name = "reina"      , .type = "uint16_t", .read = 1 , .write = 1  },
    {.number = 4  , .name = "asuka"      , .type = "int16_t" , .read = 1 , .write = 1  },
    {.number = 5  , .name = "haruka"     , .type = "uint32_t", .read = 1 , .write = 1  },
    {.number = 6  , .name = "kaori"      , .type = "int32_t" , .read = 1 , .write = 1  },
    {.number = 7  , .name = "natsuki"    , .type = "uint64_t", .read = 1 , .write = 1  },
    {.number = 8  , .name = "yuko"       , .type = "int64_t" , .read = 1 , .write = 1  },
    {.number = 9  , .name = "mizore"     , .type = "float"   , .read = 1 , .write = 1  },
    {.number = 10 , .name = "nozomi"     , .type = "float"  , .read = 1 , .write = 1  },
    {.number = 11 , .name = "shuichi"    , .type = "uint8_t" , .read = 1 , .write = 0 },
    {.number = 12 , .name = "takuya"     , .type = "uint16_t", .read = 0, .write = 1  },
    {.number = 13 , .name = "riko"       , .type = "uint32_t", .read = 1 , .write = 0 },
    {.number = 14 , .name = "aoi"        , .type = "uint64_t", .read = 0, .write = 1  },
    {.number = 15 , .name = "noboru"     , .type = "float"   , .read = 1 , .write = 0 }
  }
};
