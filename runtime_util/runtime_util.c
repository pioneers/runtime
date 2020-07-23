#include "runtime_util.h"

/*
 * Definition of each lowcar device and their params
 */
device_t LimitSwitch = {
  .type = 0,
  .name = "LimitSwitch",
  .num_params = 3,
  .params = {
    {.name = "switch0"    , .type = BOOL    , .read = 1 , .write = 0 },
    {.name = "switch1"    , .type = BOOL    , .read = 1 , .write = 0 },
    {.name = "switch2"    , .type = BOOL    , .read = 1 , .write = 0 }
  }
};

device_t LineFollower = {
  .type = 1,
  .name = "LineFollower",
  .num_params = 3,
  .params = {
    {.name = "left"       , .type = FLOAT   , .read = 1 , .write = 0 },
    {.name = "center"     , .type = FLOAT   , .read = 1 , .write = 0 },
    {.name = "right"      , .type = FLOAT   , .read = 1 , .write = 0 }
  }
};

device_t Potentiometer = {
  .type = 2,
  .name = "Potentiometer",
  .num_params = 3,
  .params = {
    {.name = "pot0"       , .type = FLOAT   , .read = 1 , .write = 0 },
    {.name = "pot1"       , .type = FLOAT   , .read = 1 , .write = 0 },
    {.name = "pot2"       , .type = FLOAT   , .read = 1 , .write = 0 }
  }
};

device_t Encoder = {
  .type = 3,
  .name = "Encoder",
  .num_params = 1,
  .params = {
    {.name = "rotation"   , .type = INT , .read = 1 , .write = 0 }
  }
};

device_t BatteryBuzzer = {
  .type = 4,
  .name = "BatteryBuzzer",
  .num_params = 8,
  .params = {
    {.name = "is_unsafe"  , .type = BOOL     , .read = 1 , .write = 0 },
    {.name = "calibrated" , .type = BOOL     , .read = 1 , .write = 0  },
    {.name = "v_cell1"    , .type = FLOAT    , .read = 1 , .write = 0 },
    {.name = "v_cell2"    , .type = FLOAT    , .read = 1 , .write = 0 },
    {.name = "v_cell3"    , .type = FLOAT    , .read = 1 , .write = 0 },
    {.name = "v_batt"     , .type = FLOAT    , .read = 1 , .write = 0 },
    {.name = "dv_cell2"   , .type = FLOAT    , .read = 1 , .write = 0 },
    {.name = "dv_cell3"   , .type = FLOAT    , .read = 1 , .write = 0 }
  }
};

device_t TeamFlag = {
  .type = 5,
  .name = "TeamFlag",
  .num_params = 7,
  .params = {
    {.name = "mode"  , .type = BOOL    , .read = 1 , .write = 1},
    {.name = "blue"  , .type = BOOL    , .read = 1 , .write = 1},
    {.name = "yellow", .type = BOOL    , .read = 1 , .write = 1},
    {.name = "led1"  , .type = BOOL    , .read = 1 , .write = 1},
    {.name = "led2"  , .type = BOOL    , .read = 1 , .write = 1},
    {.name = "led3"  , .type = BOOL    , .read = 1 , .write = 1},
    {.name = "led4"  , .type = BOOL    , .read = 1 , .write = 1}
  }
};

device_t ServoControl = {
  .type = 7,
  .name= "ServoControl",
  .num_params = 2,
  .params = {
    {.name = "servo0"     , .type = FLOAT , .read = 1 , .write = 1  },
    {.name = "servo1"     , .type = FLOAT , .read = 1 , .write = 1  }
  }
};

device_t YogiBear = {
  .type = 10,
  .name = "YogiBear",
  .num_params = 14,
  .params = {
    {.name = "duty_cycle"          , .type = FLOAT    , .read = 1 , .write = 1  },
    {.name = "pid_pos_setpoint"    , .type = FLOAT    , .read = 0 , .write = 1  },
    {.name = "pid_pos_kp"          , .type = FLOAT    , .read = 0 , .write = 1  },
    {.name = "pid_pos_ki"          , .type = FLOAT    , .read = 0 , .write = 1  },
    {.name = "pid_pos_kd"          , .type = FLOAT    , .read = 0 , .write = 1  },
    {.name = "pid_vel_setpoint"    , .type = FLOAT    , .read = 0 , .write = 1  },
    {.name = "pid_vel_kp"          , .type = FLOAT    , .read = 0 , .write = 1  },
    {.name = "pid_vel_ki"          , .type = FLOAT    , .read = 0 , .write = 1  },
    {.name = "pid_vel_kd"          , .type = FLOAT    , .read = 0 , .write = 1  },
    {.name = "current_thresh"      , .type = FLOAT    , .read = 0 , .write = 1  },
    {.name = "enc_pos"             , .type = FLOAT    , .read = 1 , .write = 1  },
    {.name = "enc_vel"             , .type = FLOAT    , .read = 1 , .write = 0  },
    {.name = "motor_current"       , .type = FLOAT    , .read = 1 , .write = 0  },
    {.name = "deadband"            , .type = FLOAT    , .read = 1 , .write = 1  }
  }
};

device_t RFID = {
  .type = 11,
  .name = "RFID",
  .num_params = 2,
  .params = {
    {.name = "id_upper"     , .type = INT     , .read = 1 , .write = 0  },
    {.name = "id_lower"     , .type = INT     , .read = 1 , .write = 0  },
    {.name = "tag_detect"   , .type = INT     , .read = 1 , .write = 0  }
  }
};

device_t PolarBear = {
  .type = 12,
  .name = "PolarBear",
  .num_params = 3,
  .params = {
    {.name = "duty_cycle"          , .type = FLOAT    , .read = 1 , .write = 1  },
    {.name = "motor_current"       , .type = FLOAT    , .read = 1 , .write = 0  },
    {.name = "deadband"            , .type = FLOAT    , .read = 1 , .write = 1  }
  }
};

device_t KoalaBear = {
  .type = 13,
  .name = "KoalaBear",
  .num_params = 16,
  .params = {
      {.name = "duty_cycle_a"     , .type = FLOAT      , .read = 1 , .write = 1  },
      {.name = "deadband_a"       , .type = FLOAT      , .read = 1 , .write = 1  },
      {.name = "current_a"        , .type = FLOAT      , .read = 1 , .write = 1  },
      {.name = "pid_enabled_a"    , .type = BOOL       , .read = 1 , .write = 1  },
      {.name = "pid_kp_a"         , .type = FLOAT      , .read = 1 , .write = 1  },
      {.name = "pid_ki_a"         , .type = FLOAT      , .read = 1 , .write = 1  },
      {.name = "pid_kd_a"         , .type = FLOAT      , .read = 1 , .write = 1  },
      {.name = "enc_a"            , .type = FLOAT      , .read = 1 , .write = 1  },
      // Same params as above except for motor b
      {.name = "duty_cycle_b"     , .type = FLOAT      , .read = 1 , .write = 1  },
      {.name = "deadband_b"       , .type = FLOAT      , .read = 1 , .write = 1  },
      {.name = "current_b"        , .type = FLOAT      , .read = 1 , .write = 1  },
      {.name = "pid_enabled_b"    , .type = BOOL       , .read = 1 , .write = 1  },
      {.name = "pid_kp_b"         , .type = FLOAT      , .read = 1 , .write = 1  },
      {.name = "pid_ki_b"         , .type = FLOAT      , .read = 1 , .write = 1  },
      {.name = "pid_kd_b"         , .type = FLOAT      , .read = 1 , .write = 1  },
      {.name = "enc_b"            , .type = FLOAT      , .read = 1 , .write = 1  },
  }
};

device_t DummyDevice = {
    .type = 14,
    .name = "DummyDevice",
    .num_params = 16,
    .params = {
        // Read-only
        {.name = "RUNTIME",     .type = INT   , .read = 1, .write = 0},
        {.name = "SHEPHERD",    .type = FLOAT , .read = 1, .write = 0},
        {.name = "DAWN",        .type = BOOL  , .read = 1, .write = 0},
        {.name = "DEVOPS",      .type = INT   , .read = 1, .write = 0},
        {.name = "ATLAS",       .type = FLOAT , .read = 1, .write = 0},
        {.name = "INFRA",       .type = BOOL  , .read = 1, .write = 0},
        // Write-only
        {.name = "SENS",        .type = INT   , .read = 0, .write = 1},
        {.name = "PDB",         .type = FLOAT , .read = 0, .write = 1},
        {.name = "MECH",        .type = BOOL  , .read = 0, .write = 1},
        {.name = "CPR",         .type = INT   , .read = 0, .write = 1},
        {.name = "EDU",         .type = FLOAT , .read = 0, .write = 1},
        {.name = "EXEC",        .type = BOOL  , .read = 0, .write = 1},
        // Read-able and write-able
        {.name = "PIEF",        .type = INT   , .read = 1, .write = 1},
        {.name = "FUNTIME",     .type = FLOAT , .read = 1, .write = 1},
        {.name = "SHEEP",       .type = BOOL  , .read = 1, .write = 1},
        {.name = "DUSK",        .type = INT   , .read = 1, .write = 1},
    }
};


/* An array that holds pointers to the structs of each lowcar device */
device_t* DEVICES[DEVICES_LENGTH] = {&LimitSwitch, &LineFollower, &Potentiometer, NULL, &BatteryBuzzer,
                                     &TeamFlag, NULL, &ServoControl, NULL, NULL,
                                     &YogiBear, &RFID, &PolarBear, &KoalaBear, &DummyDevice};

device_t* get_device(uint8_t dev_type) {
    if (0 <= dev_type && dev_type < DEVICES_LENGTH) {
       return DEVICES[dev_type];
    }
    return NULL;
}

uint8_t device_name_to_type(char* dev_name) {
    for (int i = 0; i < DEVICES_LENGTH; i++) {
        if (DEVICES[i] != NULL && strcmp(DEVICES[i]->name, dev_name) == 0) {
            return i;
        }
    }
    return -1;
}

char* get_device_name(uint8_t dev_type) {
    device_t* device = get_device(dev_type);
    if (device == NULL) {
        return NULL;
    }
    return device->name;
}

param_desc_t* get_param_desc(uint8_t dev_type, char* param_name) {
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

int8_t get_param_idx(uint8_t dev_type, char* param_name) {
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

/* Returns the number of milliseconds since the Unix Epoch */
uint64_t millis() {
	struct timeval time; // Holds the current time in seconds + microsecondsx
	gettimeofday(&time, NULL);
	uint64_t s1 = (uint64_t)(time.tv_sec) * 1000;  // Convert seconds to milliseconds
	uint64_t s2 = (time.tv_usec / 1000);		   // Convert microseconds to milliseconds
	return s1 + s2;
}

/*
 * Read n bytes from fd into buf; return number of bytes read into buf (deals with interrupts and unbuffered reads)
 * Arguments:
 *    - int fd: file descriptor to read from
 *    - void *buf: pointer to location to copy read bytes into
 *    - size_t n: number of bytes to read
 * Return:
 *    - > 0: number of bytes read into buf
 *    - 0: read EOF on fd
 *    - -1: read errored out
 */
int readn (int fd, void *buf, uint16_t n)
{
	uint16_t n_remain = n;
	uint16_t n_read;
	char *curr = buf;

	while (n_remain > 0) {
		if ((n_read = read(fd, curr, n_remain)) < 0) {
			if (errno == EINTR) { //read interrupted by signal; read again
				n_read = 0;
			} else {
				perror("read");
				return -1;
			}
		} else if (n_read == 0) { //received EOF
			return 0;
		}
		n_remain -= n_read;
		curr += n_read;
	}
	return (n - n_remain);
}

/*
 * Read n bytes from buf to fd; return number of bytes written to buf (deals with interrupts and unbuffered writes)
 * Arguments:
 *    - int fd: file descriptor to write to
 *    - void *buf: pointer to location to read from
 *    - size_t n: number of bytes to write
 * Return:
 *    - >= 0: number of bytes written into buf
 *    - -1: write errored out
 */
int writen (int fd, void *buf, uint16_t n)
{
	uint16_t n_remain = n;
	uint16_t n_written;
	char *curr = buf;

	while (n_remain > 0) {
		if ((n_written = write(fd, curr, n_remain)) <= 0) {
			if (n_written < 0 && errno == EINTR) { //write interrupted by signal, write again
				n_written = 0;
			} else {
				perror("write");
				return -1;
			}
		}
		n_remain -= n_written;
		curr += n_written;
	}
	return n;
}
