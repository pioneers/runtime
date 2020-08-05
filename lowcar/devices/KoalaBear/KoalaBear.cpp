#include "KoalaBear.h"

//********************************* constants and variables used for setting up the controller ********************//
//TODO: ask electrical for what they mean
#define CTRL_REG      0b000
#define TORQUE_REG    0b001

// Control Register options
// Dead time
#define DTIME_410_ns  0b00
#define DTIME_460_ns  0b01
#define DTIME_670_ns  0b10
#define DTIME_880_ns  0b11

// Current sense gain
#define ISGAIN_5      0b00
#define ISGAIN_10     0b01
#define ISGAIN_20     0b10
#define ISGAIN_40     0b11

// Enable or disable motors
#define ENABLE        0b1
#define DISABLE       0b0

// Set VREF scaling out of 256
#define TORQUE        0x70

uint16_t ctrl_read_data;
uint16_t torque_read_data;

volatile int32_t enc_a, enc_b;

//********************************** labels for useful indices **********************************//
typedef enum {
    // params for the first motor
    DUTY_CYCLE_A  = 0, // Student's desired speed between -1 and 1, inclusive
    DEADBAND_A    = 1, // between 0 and 1, magnitude of duty cycle input under which the motor will not move
    CURRENT_A     = 2,
    PID_ENABLED_A = 3, // True if using PID control; False if using manual drive mode
    PID_KP_A      = 4, // these three are the PID coefficients
    PID_KI_A      = 5,
    PID_KD_A      = 6,
    ENC_A         = 7, // encoder position, in ticks
    //same params for the second motor
    DUTY_CYCLE_B  = 8,
    DEADBAND_B    = 9,
    CURRENT_B     = 10,
    PID_ENABLED_B = 11,
    PID_KP_B      = 12,
    PID_KI_B      = 13,
    PID_KD_B      = 14,
    ENC_B         = 15
} param;

typedef enum {
    MTRA = 1,
    MTRB = 0,
} mtrs;

//******************************* ENCODER TICK INTERRPUT HANDLERS ***************************//

// TODO: write a descriptive comment here once we know this works
static void handle_enc_a_tick() {
    if (digitalRead(AENC2)) {
        enc_a++;
    } else {
        enc_a--;
    }
}

// TODO: write a descriptive comment here once we know this works
static void handle_enc_b_tick() {
    if (digitalRead(BENC2)) {
        enc_b++;
    } else {
        enc_b--;
    }
}

//*********************************** MAIN KOALABEAR CODE ***********************************//

KoalaBear::KoalaBear() : Device(DeviceType::KOALA_BEAR, 13) {
    // initialize motor values    
    this->target_speed_a = this->target_speed_b = 0.0;
    this->deadband_a = this->deadband_b = 0.05;
    
    // initialize encoders
    enc_a = enc_b = 0.0;
    attachInterrupt(digitalPinToInterrupt(AENC1), handle_enc_a_tick, RISING); // set interrupt service routines for encoder pins
    attachInterrupt(digitalPinToInterrupt(BENC1), handle_enc_b_tick, RISING);
    
    // initialize PID controllers
    this->pid_a = new PID();
    this->pid_b = new PID();
    this->pid_enabled_a = this->pid_enabled_b = FALSE; // TODO: change to true when PID written
    
    this->led = new LEDKoala();
    this->prev_led_time = millis();
    this->curr_led_mtr = MTRA;
}

size_t KoalaBear::device_read(uint8_t param, uint8_t *data_buf) {
    float *float_buf = (float *) data_buf;
    bool *bool_buf = (bool *) data_buf;
    int32_t *int_buf = (int32_t *) data_buf;

    switch (param) {
        case DUTY_CYCLE_A:
            float_buf[0] = this->target_speed_a;
            return sizeof(float);
        case DEADBAND_A:
            float_buf[0] = this->deadband_a;
            return sizeof(float);
        case CURRENT_A:
            // TODO
            return sizeof(float);
        case PID_ENABLED_A:
            float_buf[0] = this->pid_enabled_a;
            return sizeof(uint8_t);
        case PID_KP_A:
            float_buf[0] = this->pid_a->get_kp();
            return sizeof(float);
        case PID_KI_A:
            float_buf[0] = this->pid_a->get_ki();
            return sizeof(float);
        case PID_KD_A:
            float_buf[0] = this->pid_a->get_kd();
            return sizeof(float);
        case ENC_A:
            int_buf[0] = enc_a;
            return sizeof(int32_t);
            
        // Params for Motor B
        case DUTY_CYCLE_B:
            float_buf[0] = this->target_speed_b;
            return sizeof(float);
        case DEADBAND_B:
            float_buf[0] = this->deadband_b;
            return sizeof(float);
        case CURRENT_B:
            // TODO
            return sizeof(float);
        case PID_ENABLED_B:
            data_buf[0] = this->pid_enabled_b;
            return sizeof(uint8_t);
        case PID_KP_B:
            float_buf[0] = this->pid_b->get_kp();
            return sizeof(float);
        case PID_KI_B:
            float_buf[0] = this->pid_b->get_ki();
            return sizeof(float);
        case PID_KD_B:
            float_buf[0] = this->pid_b->get_kd();
            return sizeof(float);
        case ENC_B:
            int_buf[0] = enc_b;
            return sizeof(int32_t);
    }
    return 0;
}

size_t KoalaBear::device_write(uint8_t param, uint8_t *data_buf) {
    switch (param) {
        case DUTY_CYCLE_A:
            this->pid_enabled_a = FALSE; // TODO: remove later once PID added in
            this->target_speed_a = ((float *)data_buf)[0];
            return sizeof(float);
        case DEADBAND_A:
            this->deadband_a = ((float *)data_buf)[0];
            return sizeof(float);
        case CURRENT_A:
            break;
        case PID_ENABLED_A:
            this->pid_enabled_a = data_buf[0];
            return sizeof(uint8_t);
        case PID_KP_A:
            this->pid_a->set_coefficients(((double *)data_buf)[0], this->pid_a->get_ki(), this->pid_a->get_kd());
            return sizeof(float);
        case PID_KI_A:
            this->pid_a->set_coefficients(this->pid_a->get_kp(), ((double *)data_buf)[0], this->pid_a->get_kd());
            return sizeof(float);
        case PID_KD_A:
            this->pid_a->set_coefficients(this->pid_a->get_kp(), this->pid_a->get_ki(), ((double *)data_buf)[0]);
            return sizeof(float);
        case ENC_A:
            enc_a = ((int32_t *)data_buf)[0];
            this->pid_a->set_pos_to((float) enc_a);
            return sizeof(int32_t);
            
        // Params for Motor B
        case DUTY_CYCLE_B:
            this->pid_enabled_b = FALSE; //remove later for PID functionality
            this->target_speed_b = ((float *)data_buf)[0];
            return sizeof(float);
        case DEADBAND_B:
            this->deadband_b = ((float *)data_buf)[0];
            return sizeof(float);
        case CURRENT_B:
            break;
        case PID_ENABLED_B:
            this->pid_enabled_b = data_buf[0];
            return sizeof(uint8_t);
        case PID_KP_B:
            this->pid_b->set_coefficients(((double *)data_buf)[0], this->pid_b->get_ki(), this->pid_b->get_kd());
            return sizeof(float);
        case PID_KI_B:
            this->pid_b->set_coefficients(this->pid_b->get_kp(), ((double *)data_buf)[0], this->pid_b->get_kd());
            return sizeof(float);
        case PID_KD_B:
            this->pid_b->set_coefficients(this->pid_b->get_kp(), this->pid_b->get_ki(), ((double *)data_buf)[0]);
            return sizeof(float);
        case ENC_B:
            enc_b = ((int32_t *)data_buf)[0];
            this->pid_b->set_pos_to((float) enc_b);
            return sizeof(int32_t);
        default:
            return 0;
    }
    return 0;
}

void KoalaBear::device_enable () {
    electrical_setup(); // ask electrical about this function (it's hardware setup for the motor controller)
    
    // set up encoders
    pinMode(AENC1, INPUT);
    pinMode(AENC2, INPUT);
    pinMode(BENC1, INPUT);
    pinMode(BENC2, INPUT);
    
    this->pid_enabled_a = FALSE; // TODO: change to true once implemented
    this->pid_enabled_b = FALSE;
    
    this->led->setup_LEDs();
    this->led->test_LEDs();
    
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(BIN1, OUTPUT);
    pinMode(BIN2, OUTPUT);
}

void KoalaBear::device_disable () {
    // digitalWrite(SLEEP, HIGH);
    //this->pid->setCoefficients(1, 0, 0);
    //this->pid->resetEncoder();
    
    this->pid_a->set_coefficients(0.0, 0.0, 0.0);
    this->pid_b->set_coefficients(0.0, 0.0, 0.0);
    
    this->target_speed_a = 0.0;
    this->target_speed_b = 0.0;
    this->pid_enabled_a = FALSE;
    this->pid_enabled_b = FALSE;
}

void KoalaBear::device_actions () {
    float target_A, target_B;
    unsigned long curr_time = millis();
    
    // switch between displaying info about MTRA and MTRB every 2 seconds
    if (curr_time - this->prev_led_time > 2000) {
        this->curr_led_mtr = (this->curr_led_mtr == MTRA) ? MTRB : MTRA;
        this->prev_led_time = curr_time;
    }
    if (this->curr_led_mtr == MTRA) {
        this->led->ctrl_LEDs(this->target_speed_a, this->deadband_a, TRUE);
    } else {
        this->led->ctrl_LEDs(this->target_speed_b, this->deadband_b, TRUE);
    }
    
    if (this->pid_enabled_a) {
        this->pid_a->set_target_speed(this->target_speed_a);
        target_A = this->pid_a->compute((float) enc_a);
    } else {
        target_A = this->target_speed_a;
    }

    if (this->pid_enabled_b) {
        this->pid_b->set_target_speed(this->target_speed_b);
        target_B = this->pid_b->compute((float) enc_b);
    } else {
        target_B = this->target_speed_b;
    }

    digitalWrite(SLEEP, HIGH);
    digitalWrite(RESET, LOW);

    // send target duty cycle to drive function
    drive(target_A * -1.0, MTRA); // by default, MTRA drives the wrong way
    drive(target_B, MTRB);
    
    delay(1); // delay 1 millis to slow down the loop
}

//************************* KOALABEAR HELPER FUNCTIONS *************************//

// used for setup; ask electrical for how it works / what it does
void KoalaBear::write_current_lim() {
    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
    digitalWrite(SCS, HIGH);
    SPI.transfer16((0 << 15) + (CTRL_REG << 12) + (DTIME_410_ns << 10) + (ISGAIN_20 << 8) + ENABLE);
    SPI.endTransaction();
    digitalWrite(SCS, LOW);
    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
    digitalWrite(SCS, HIGH);
    SPI.transfer16((0 << 15) + (TORQUE_REG << 12) + TORQUE);
    SPI.endTransaction();
    digitalWrite(SCS, LOW);
}

// used for setup; ask electrical for how it works / what it does
void KoalaBear::read_current_lim() {
    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
    digitalWrite(SCS, HIGH);
    ctrl_read_data = (uint16_t) SPI.transfer16((1 << 15) + (CTRL_REG << 12));
    SPI.endTransaction();
    digitalWrite(SCS, LOW);
    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
    digitalWrite(SCS, HIGH);
    torque_read_data = (uint16_t) SPI.transfer16((1 << 15) + (TORQUE_REG << 12));
    SPI.endTransaction();
    digitalWrite(SCS, LOW);
}

// used for setup; ask electrical for how it works / what it does
void KoalaBear::electrical_setup() {
    SPI.begin();

    pinMode(HEARTBEAT, OUTPUT);
    pinMode(RESET, OUTPUT);
    pinMode(SLEEP, OUTPUT);
    pinMode(SCS, OUTPUT);

    delay(100);

    digitalWrite(SLEEP, HIGH);
    digitalWrite(RESET, LOW);

    digitalWrite(HEARTBEAT, HIGH);
    digitalWrite(SCS, LOW);

    write_current_lim();
    read_current_lim();
}

/*
 * Given a TARGET between -1 and 1 inclusive, and a motor MTR,
 * analogWrite to the appropriate pins to accelerate/decelerate
 * Negative indicates moving backwards; Positive indicates moving forwards.
 * If moving forwards, set pwm2 to 255 (stop), then move pwm1 down
 * If moving backwards, set pwm1 to 255, then move pwm2 down
 * Make sure that at least one of the pins is set to 255 at all times.
 */
void KoalaBear::drive (float target, uint8_t mtr) {
    int pin1 = (mtr == MTRA) ? AIN1 : BIN1; // select the correct pins based on the motor
    int pin2 = (mtr == MTRA) ? AIN2 : BIN2;

    float direction = (target > 0.0) ? 1.0 : -1.0;
    int currpwm1 = 255;
    int currpwm2 = 255;

    // Determine how much to move the pins down from 255
    // Sanity check: If |target| == 1, move max speed --> pwm_difference == 255
    //				 If |target| == 0, stop moving	  --> pwm_difference == 0
    int pwm_difference = (int)(target * direction * 255.0); // A number between 0 and 255 inclusive (255 is stop; 0 is max speed);

    if (direction > 0) { // Moving forwards
        currpwm1 -= pwm_difference;
    } else if (direction < 0) { // Moving backwards
        currpwm2 -= pwm_difference;
    }

    analogWrite(pin1, currpwm1);
    analogWrite(pin2, currpwm2);
}
