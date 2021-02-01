#include "KoalaBear.h"

//********************************* constants and variables used for setting up the controller ********************//
//TODO: ask electrical for what they mean
#define CTRL_REG 0b000
#define TORQUE_REG 0b001

// Control Register options
// Dead time
#define DTIME_410_ns 0b00
#define DTIME_460_ns 0b01
#define DTIME_670_ns 0b10
#define DTIME_880_ns 0b11

// Current sense gain
#define ISGAIN_5 0b00
#define ISGAIN_10 0b01
#define ISGAIN_20 0b10
#define ISGAIN_40 0b11

// Enable or disable motors
#define ENABLE 0b1
#define DISABLE 0b0

// Set VREF scaling out of 256
#define TORQUE 0x70

uint16_t ctrl_read_data;
uint16_t torque_read_data;

//******************************* KOALABEAR CONSTANTS AND PARAMS ****************************//

// ----------------------> CHANGE THIS TO IMPLEMENT MAX SPEED FOR KARTS <----------------------
#define MAX_DUTY_CYCLE 1.0  // maximum duty cycle to cap how fast the motors can go: range (0, 1]
// --------------------------------------------------------------------------------------------

// --------------------> CHANGE THIS TO IMPLEMENT ACCELERATION FOR KARTS <---------------------
#define ACCEL 1.0			// acceleration of motors, in duty cycle units per second squared
// --------------------------------------------------------------------------------------------

// default values for PID controllers; PID control is explained in the Wiki!
#define KP_DEFAULT 0.25
#define KI_DEFAULT 0.0
#define KD_DEFAULT 0.0

typedef enum {
    // params for the first motor
    VELOCITY_A = 0,     // Student's desired velocity between -1.0 and 1.0, inclusive
    DEADBAND_A = 1,     // Between 0 and 1, magnitude of duty cycle input under which the motor will not move
    INVERT_A = 2,       // True if Motor A should rotate in opposite direction of default; False for default direction
    PID_ENABLED_A = 3,  // True if using PID control; False if using manual drive mode
    PID_KP_A = 4,       // these three are the PID coefficients
    PID_KI_A = 5,
    PID_KD_A = 6,
    ENC_A = 7,  // encoder position, in ticks

    // same params for the second motor
    VELOCITY_B = 8,
    DEADBAND_B = 9,
    INVERT_B = 10,
    PID_ENABLED_B = 11,
    PID_KP_B = 12,
    PID_KI_B = 13,
    PID_KD_B = 14,
    ENC_B = 15
} param;

typedef enum {
    MTRA = 0,
    MTRB = 1,
} mtrs;

//******************************* ENCODER TICK INTERRPUT HANDLERS ***************************//
// holds current encoder values of motors; they're volatile because all variables used
// in interrupt service routines must be volatile
volatile int32_t enc_a, enc_b;

// These functions are interrupt service routines (ISR) and are basically functions
// that are called whenever something happens on a particular pin. In this case,
// the function is called when the Arduino senses a rising edge on one of the
// encoder pins. Depending on whether the other encoder pin is high or low, we can
// deduce the direction of the rotation and therefore increment or decrement the
// encoder tick count accordingly. You can see the interrupt triggers being set in
// the KoalaBear device constructor (setting to call these functions when one of the
// encoder pins encounters a rising edge).
static void handle_enc_a_tick() {
    if (digitalRead(AENC2)) {
        enc_a++;
    } else {
        enc_a--;
    }
}

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
    this->velocity_a = this->velocity_b = 0.0;
    this->deadband_a = this->deadband_b = 0.05;
    this->invert_a = this->invert_b = FALSE;  // by default, motor directions are not inverted
	this->curr_velocity_a = this->curr_velocity_b = 0.0;

    // initialize encoders
    enc_a = enc_b = 0.0;
    attachInterrupt(digitalPinToInterrupt(AENC1), handle_enc_a_tick, RISING);  // set interrupt service routines for encoder pins
    attachInterrupt(digitalPinToInterrupt(BENC1), handle_enc_b_tick, RISING);

    // initialize PID controllers
    this->pid_a = new PID();
    this->pid_b = new PID();
    this->pid_enabled_a = this->pid_enabled_b = FALSE;  // by default, PID control is enabled
    this->pid_a->set_coefficients(KP_DEFAULT, KI_DEFAULT, KD_DEFAULT);
    this->pid_b->set_coefficients(KP_DEFAULT, KI_DEFAULT, KD_DEFAULT);

    this->led = new LEDKoala();
    this->prev_led_time = this->prev_loop_time = millis();
    this->curr_led_mtr = MTRA;
	this->online = FALSE;
}

size_t KoalaBear::device_read(uint8_t param, uint8_t* data_buf) {
    float* float_buf = (float*) data_buf;
    bool* bool_buf = (bool*) data_buf;
    int32_t* int_buf = (int32_t*) data_buf;

    switch (param) {
            // Params for Motor A
        case VELOCITY_A:
            float_buf[0] = this->velocity_a;
            return sizeof(float);
        case DEADBAND_A:
            float_buf[0] = this->deadband_a;
            return sizeof(float);
        case INVERT_A:
            bool_buf[0] = this->invert_a;
            return sizeof(uint8_t);
        case PID_ENABLED_A:
            bool_buf[0] = this->pid_enabled_a;
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
        case VELOCITY_B:
            float_buf[0] = this->velocity_b;
            return sizeof(float);
        case DEADBAND_B:
            float_buf[0] = this->deadband_b;
            return sizeof(float);
        case INVERT_B:
            bool_buf[0] = this->invert_b;
            return sizeof(uint8_t);
        case PID_ENABLED_B:
            bool_buf[0] = this->pid_enabled_b;
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

size_t KoalaBear::device_write(uint8_t param, uint8_t* data_buf) {
    switch (param) {
            // Params for Motor A
        case VELOCITY_A:
            this->velocity_a = ((float*) data_buf)[0];
            // limit to range [-1.0, 1.0]
            this->velocity_a = (this->velocity_a > 1.0) ? 1.0 : this->velocity_a;
            this->velocity_a = (this->velocity_a < -1.0) ? -1.0 : this->velocity_a;
            return sizeof(float);
        case DEADBAND_A:
            this->deadband_a = ((float*) data_buf)[0];
            return sizeof(float);
        case INVERT_A:
            this->invert_a = data_buf[0];
            return sizeof(uint8_t);
        case PID_ENABLED_A:
            this->pid_enabled_a = data_buf[0];
            return sizeof(uint8_t);
        case PID_KP_A:
            this->pid_a->set_coefficients(((double*) data_buf)[0], this->pid_a->get_ki(), this->pid_a->get_kd());
            return sizeof(float);
        case PID_KI_A:
            this->pid_a->set_coefficients(this->pid_a->get_kp(), ((double*) data_buf)[0], this->pid_a->get_kd());
            return sizeof(float);
        case PID_KD_A:
            this->pid_a->set_coefficients(this->pid_a->get_kp(), this->pid_a->get_ki(), ((double*) data_buf)[0]);
            return sizeof(float);
        case ENC_A:
            enc_a = ((int32_t*) data_buf)[0];
            this->pid_a->set_position((float) enc_a);
            return sizeof(int32_t);

        // Params for Motor B
        case VELOCITY_B:
            this->velocity_b = ((float*) data_buf)[0];
            // limit to range [-1.0, 1.0]
            this->velocity_b = (this->velocity_b > 1.0) ? 1.0 : this->velocity_b;
            this->velocity_b = (this->velocity_b < -1.0) ? -1.0 : this->velocity_b;
            return sizeof(float);
        case DEADBAND_B:
            this->deadband_b = ((float*) data_buf)[0];
            return sizeof(float);
        case INVERT_B:
            this->invert_b = data_buf[0];
            return sizeof(uint8_t);
        case PID_ENABLED_B:
            this->pid_enabled_b = data_buf[0];
            return sizeof(uint8_t);
        case PID_KP_B:
            this->pid_b->set_coefficients(((double*) data_buf)[0], this->pid_b->get_ki(), this->pid_b->get_kd());
            return sizeof(float);
        case PID_KI_B:
            this->pid_b->set_coefficients(this->pid_b->get_kp(), ((double*) data_buf)[0], this->pid_b->get_kd());
            return sizeof(float);
        case PID_KD_B:
            this->pid_b->set_coefficients(this->pid_b->get_kp(), this->pid_b->get_ki(), ((double*) data_buf)[0]);
            return sizeof(float);
        case ENC_B:
            enc_b = ((int32_t*) data_buf)[0];
            this->pid_b->set_position((float) enc_b);
            return sizeof(int32_t);
        default:
            return 0;
    }
    return 0;
}

void KoalaBear::device_enable() {
    electrical_setup();  // ask electrical about this function (it's hardware setup for the motor controller)

    // set up encoders
    pinMode(AENC1, INPUT);
    pinMode(AENC2, INPUT);
    pinMode(BENC1, INPUT);
    pinMode(BENC2, INPUT);

    this->pid_enabled_a = TRUE;
    this->pid_enabled_b = TRUE;

    this->led->setup_LEDs();
    this->led->test_LEDs();

    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(BIN1, OUTPUT);
    pinMode(BIN2, OUTPUT);
}

void KoalaBear::device_reset() {
    this->pid_a->set_coefficients(KP_DEFAULT, KI_DEFAULT, KD_DEFAULT);
    this->pid_b->set_coefficients(KP_DEFAULT, KI_DEFAULT, KD_DEFAULT);

    this->velocity_a = 0.0;
    this->velocity_b = 0.0;
	this->curr_velocity_a = 0.0;
	this->curr_velocity_b = 0.0;
    this->pid_enabled_a = TRUE;
    this->pid_enabled_b = TRUE;
	this->online = FALSE;
}

void KoalaBear::device_actions() {
	// if we just got started looping, save a value for prev_loop_time and immediately return
	// prevents extremely large interval_secs (and therefore large extraneous motions on startup)
	if (!this->online) {
		this->prev_loop_time = millis();
		this->online = TRUE;
		return;
	}
	
    float duty_cycle_a, duty_cycle_b, adjusted_velocity_a, adjusted_velocity_b;
    unsigned long curr_time = millis();
	float interval_secs = ((float) (curr_time - this->prev_loop_time)) / 1000.0;		// compute time passed between loops, in seconds

    // switch between displaying info about MTRA and MTRB every 2 seconds
    if (curr_time - this->prev_led_time > 2000) {
        this->curr_led_mtr = (this->curr_led_mtr == MTRA) ? MTRB : MTRA;
        this->prev_led_time = curr_time;
    }
    if (this->curr_led_mtr == MTRA) {
        this->led->ctrl_LEDs(this->velocity_a, this->deadband_a, TRUE);
    } else {
        this->led->ctrl_LEDs(this->velocity_b, this->deadband_b, TRUE);
    }

    // compute the actual target speed of motors (depending on whether direction is inverted, max duty cycle, and acceleration)
	// acceleration calculation
	this->curr_velocity_a = this->curr_velocity_a + (ACCEL * ((this->velocity_a - this->curr_velocity_a > 0) ? 1.0 : -1.0) * interval_secs);
	this->curr_velocity_b = this->curr_velocity_b + (ACCEL * ((this->velocity_b - this->curr_velocity_b > 0) ? 1.0 : -1.0) * interval_secs);
		
	// inversion and max duty cycle calculation
    adjusted_velocity_a = MAX_DUTY_CYCLE * ((this->invert_a) ? this->curr_velocity_a * -1.0 : this->curr_velocity_a);
    adjusted_velocity_b = MAX_DUTY_CYCLE * ((this->invert_b) ? this->curr_velocity_b * -1.0 : this->curr_velocity_b);
	
	// this logic makes sure that the robot doesn't oscillate around 0 when the student wants the robot to stop
	if (this->velocity_a == 0.0 && adjusted_velocity_a < this->deadband_a && adjusted_velocity_a > (this->deadband_a * -1.0)) {
		this->curr_velocity_a = adjusted_velocity_a = 0.0;
	}
	if (this->velocity_b == 0.0 && adjusted_velocity_b < this->deadband_b && adjusted_velocity_b > (this->deadband_b * -1.0)) {
		this->curr_velocity_b = adjusted_velocity_b = 0.0;
	}
	
    // compute the actual duty cycle to command the motors at (depending on whether pid is enabled)
    if (this->pid_enabled_a) {
        this->pid_a->set_velocity(adjusted_velocity_a);
        duty_cycle_a = this->pid_a->compute((float) enc_a);
    } else {
        duty_cycle_a = adjusted_velocity_a;
    }

    if (this->pid_enabled_b) {
        this->pid_b->set_velocity(adjusted_velocity_b);
        duty_cycle_b = this->pid_b->compute((float) enc_b);
    } else {
        duty_cycle_b = adjusted_velocity_b;
    }

    // pins that we write on each loop to ensure we don't break stuff (ask electrical)
    digitalWrite(SLEEP, HIGH);
    digitalWrite(RESET, LOW);

    // send computed duty cycle commands to motors
    drive(duty_cycle_a, MTRA);
    drive(duty_cycle_b, MTRB);
	
	// update prev_loop_time
	this->prev_loop_time = curr_time;
}

//************************* KOALABEAR HELPER FUNCTIONS *************************//

void KoalaBear::drive(float target, uint8_t mtr) {
    int pin1 = (mtr == MTRA) ? AIN1 : BIN1;  // select the correct pins based on the motor
    int pin2 = (mtr == MTRA) ? AIN2 : BIN2;

    float direction = (target > 0.0) ? 1.0 : -1.0;
    /* If moving forwards, set pwm2 to 255 (stop), then move pwm1 down
     * If moving backwards, set pwm1 to 255, then move pwm2 down
     * Make sure that at least one of the pins is set to 255 at all times.
     */
    int currpwm1 = 255;
    int currpwm2 = 255;

    // Determine how much to move the pins down from 255
    // Sanity check: If |target| == 1, move max speed --> pwm_difference == 255
    //               If |target| == 0, stop moving    --> pwm_difference == 0
    int pwm_difference = (int) (target * direction * 255.0);  // A number between 0 and 255 inclusive (255 is stop; 0 is max speed);
    pwm_difference = (pwm_difference < 0) ? 0 : pwm_difference;
    pwm_difference = (pwm_difference > 255) ? 255 : pwm_difference;

    if (direction > 0) {  // Moving forwards
        currpwm1 -= pwm_difference;
    } else if (direction < 0) {  // Moving backwards
        currpwm2 -= pwm_difference;
    }

    analogWrite(pin1, currpwm1);
    analogWrite(pin2, currpwm2);
}

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
