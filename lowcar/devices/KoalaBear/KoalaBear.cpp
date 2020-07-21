#include "KoalaBear.h"

//********************************* constants and variables used for setting up the controller ********************//
//TODO: ask electrical for what they mean
#define CTRL_REG      0b000
#define TORQUE_REG    0b001

//Control Register options
//Dead time
#define DTIME_410_ns  0b00
#define DTIME_460_ns  0b01
#define DTIME_670_ns  0b10
#define DTIME_880_ns  0b11

//Current sense gain
#define ISGAIN_5      0b00
#define ISGAIN_10     0b01
#define ISGAIN_20     0b10
#define ISGAIN_40     0b11

//Enable or disable motors
#define ENABLE        0b1
#define DISABLE       0b0

//Set VREF scaling out of 256
#define TORQUE        0x70

uint16_t ctrl_read_data;
uint16_t torque_read_data;

//********************************** labels for useful indices **********************************//
typedef enum {
	//params for the first motor
	DUTY_CYCLE_A	= 0,	//Student's desired speed between -1 and 1, inclusive
	DEADBAND_A		= 1,	//between 0 and 1, magnitude of duty cycle input under which the motor will not move
	CURRENT_A		= 2,
	PID_ENABLED_A	= 3, 	//True if using PID control; False if using manual drive mode
	PID_KP_A		= 4,	//these three are the PID coefficients
	PID_KI_A		= 5,
	PID_KD_A		= 6,
	ENC_A			= 7,	//encoder position, in ticks
	//same params for the second motor
	DUTY_CYCLE_B	= 8,
	DEADBAND_B		= 9,
	CURRENT_B		= 10,
	PID_ENABLED_B	= 11,
	PID_KP_B		= 12,
	PID_KI_B		= 13,
	PID_KD_B		= 14,
	ENC_B			= 15
} param;

typedef enum {
	MTRA = 1,
	MTRB = 0,
} mtrs;

//*********************************** MAIN KOALABEAR CODE ***********************************//

KoalaBear::KoalaBear () : Device (DeviceID::KOALA_BEAR, 13), //,  encdr(encoder0PinA, encoder0PinB), pid(0.0, 0.0, 0.0, 0.0, (double) millis(), encdr)
{
	this->desired_speeds = (float *) calloc(2, sizeof(float));	// "duty_cycle"

	this->deadbands = (float *) calloc(2, sizeof(float));
	this->deadbands[MTRA] = 0.05;
	this->deadbands[MTRB] = 0.05;

	//this->pid = new PID(0.0, 1.0, 0.0, 0.0, (float) millis());
	this->pid_enabled = (uint8_t *) calloc(2, sizeof(uint8_t));

	this->prev_led_time = millis();
	this->curr_led_mtr = MTRA;

	this->led = new LEDKoala();
}

size_t KoalaBear::device_read (uint8_t param, uint8_t *data_buf)
{
	float *float_buf = (float *) data_buf;
	bool *bool_buf = (bool *) data_buf;

	switch (param) {

		case DUTY_CYCLE_A:
			float_buf[0] = this->desired_speeds[MTRA];
			return sizeof(float);

		case DEADBAND_A:
			float_buf[0] = this->deadbands[MTRA];
			return sizeof(float);

		case CURRENT_A:
			// TODO
			return sizeof(float);

		case PID_ENABLED_A:
			float_buf[0] = this->pid_enabled[MTRA];
			return sizeof(uint8_t);

		//TODO: ask PID controller for motor A for KP
		case PID_KP_A:
			return sizeof(float);

		//TODO: ask PID controller for motor A for KI
		case PID_KI_A:
			return sizeof(float);

		//TODO: ask PID controller for motor A for KD
		case PID_KD_A:
			return sizeof(float);

		case ENC_A:
			//float_buf[0] = this->pid->readPos();
			return sizeof(float);

		// Params for Motor B

		case DUTY_CYCLE_B:
			float_buf[0] = this->desired_speeds[MTRB];
			return sizeof(float);

		case DEADBAND_B:
			float_buf[0] = this->deadbands[MTRB];
			return sizeof(float);

		case CURRENT_B:
			// TODO
			return sizeof(float);

		case PID_ENABLED_B:
			data_buf[0] = this->pid_enabled[MTRB];
			return sizeof(uint8_t);

		//TODO: ask PID controller for motor B for KP
		case PID_KP_B:
			return sizeof(float);

		//TODO: ask PID controller for motor B for KI
		case PID_KI_B:
			return sizeof(float);

		//TODO: ask PID controller for motor B for KD
		case PID_KD_B:
			return sizeof(float);

		case ENC_B:
			//float_buf[0] = this->pid->readPos();
			return sizeof(float);
	}
	return 0;
}

size_t KoalaBear::device_write (uint8_t param, uint8_t *data_buf)
{
  switch (param) {

		case DUTY_CYCLE_A:
			this->pid_enabled[MTRA] = FALSE; // TODO: remove later once PID added in
			this->desired_speeds[MTRA] = ((float *)data_buf)[0];
			return sizeof(float);

		case DEADBAND_A:
			this->deadbands[MTRA] = ((float *)data_buf)[0];
			return sizeof(float);

		case CURRENT_A:
			break;

		case PID_ENABLED_A:
			this->pid_enabled[MTRA] = data_buf[0];
			return sizeof(uint8_t);

		case PID_KP_A:
			//this->pid->setCoefficients(((double *)data_buf)[0], this->pid->getKI(), this->pid->getKD());
			return sizeof(float);

		case PID_KI_A:
			//this->pid->setCoefficients(this->pid->getKP(), ((double *)data_buf)[0], this->pid->getKD());
			return sizeof(float);

		case PID_KD_A:
			//this->pid->setCoefficients(this->pid->getKP(), this->pid->getKI(), ((double *)data_buf)[0]);
			return sizeof(float);

		case ENC_A:
		/* TODO
			if ((float) data_buf[0] == 0) {
				this->pid->resetEncoder();
				return sizeof(float);
			}
		*/
			return sizeof(float);

		case DUTY_CYCLE_B:
			this->pid_enabled[MTRB] = FALSE; //remove later for PID functionality
			this->desired_speeds[MTRB] = ((float *)data_buf)[0];
			return sizeof(float);

		case DEADBAND_B:
			this->deadbands[MTRB] = ((float *)data_buf)[0];
			return sizeof(float);

		case CURRENT_B:
			break;

		case PID_ENABLED_B:
			this->pid_enabled[MTRB] = data_buf[0];
			return sizeof(uint8_t);

		case PID_KP_B:
			//this->pid->setCoefficients(((double *)data_buf)[0], this->pid->getKI(), this->pid->getKD());
			return sizeof(float);

		case PID_KI_B:
			//this->pid->setCoefficients(this->pid->getKP(), ((double *)data_buf)[0], this->pid->getKD());
			return sizeof(float);

		case PID_KD_B:
			//this->pid->setCoefficients(this->pid->getKP(), this->pid->getKI(), ((double *)data_buf)[0]);
			return sizeof(float);

		case ENC_B:
		/* TODO
			if ((float) data_buf[0] == 0) {
				this->pid->resetEncoder();
				return sizeof(float);
			}
		*/
			return sizeof(float);

		default:
			return 0;
	}
	return 0;
}

void KoalaBear::device_enable ()
{
	electrical_setup(); //ask electrical about this function (it's hardware setup for the motor controller_

    //this->pid->encoderSetup();
    this->led->setup_LEDs();
    this->led->test_LEDs();
    this->pid_enabled[MTRA] = FALSE; //change to true once implemented
	this->pid_enabled[MTRB] = FALSE;

	//pinMode(feedback,INPUT);
	pinMode(AIN1, OUTPUT);
	pinMode(AIN2, OUTPUT);
	pinMode(BIN1, OUTPUT);
	pinMode(BIN2, OUTPUT);

	this->desired_speeds[MTRA] = 0.0;
	this->desired_speeds[MTRB] = 0.0;
}

void KoalaBear::device_disable ()
{
	// digitalWrite(SLEEP, HIGH);
	//this->pid->setCoefficients(1, 0, 0);
	//this->pid->resetEncoder();

	this->desired_speeds[MTRA] = 0.0;
	this->desired_speeds[MTRB] = 0.0;
	this->pid_enabled[MTRA] = FALSE;
	this->pid_enabled[MTRB] = FALSE;
}

void KoalaBear::device_actions ()
{
	float target_A, target_B;

	//switch between displaying info about MTRA and MTRB every 2 seconds
	if (millis() - this->prev_led_time > 2000) {
		this->curr_led_mtr = (this->curr_led_mtr == MTRA) ? MTRB : MTRA;
		this->prev_led_time = millis();
	}
	this->led->ctrl_LEDs(this->desired_speeds[this->curr_led_mtr], TRUE); // TODO: Consider changing second arg to a condition

	if (this->pid_enabled[MTRA]) {
		//this->pid->setSetpoint(this->desired_speeds[MTRA]);
		//target = this->pid->compute();
		target_A = 0.0;
	} else {
		target_A = this->desired_speeds[MTRA];
	}

	if (this->pid_enabled[MTRB]) {
		//this->pid->setSetpoint(this->desired_speeds[MTRB]);
		//target = this->pid->compute();
		target_B = 0.0;
	} else {
		target_B = this->desired_speeds[MTRB];
	}

	digitalWrite(SLEEP, HIGH);
	digitalWrite(RESET, LOW);

	//send target duty cycle to drive function
	drive(target_A, MTRA);
	drive(target_B *-1.0, MTRB); //MTRB by default moves in the opposite direction as motor A
}

//************************* KOALABEAR HELPER FUNCTIONS *************************//
//used for setup; ask electrical for how it works / what it does
void KoalaBear::write_current_lim()
{
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

//used for setup; ask electrical for how it works / what it does
void KoalaBear::read_current_lim()
{
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

//used for setup; ask electrical for how it works / what it does
void KoalaBear::electrical_setup()
{
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

/* Calculates the sign of a float. */
int KoalaBear::sign (float x) {
    if (x > 0) {
        return 1;
    } else if (x < 0) {
        return -1;
    }
    return 0;
}

/* Given a TARGET between -1 and 1 inclusive, and a motor MTR,
** analogWrite to the appropriate pins to accelerate/decelerate
** Negative indicates moving backwards; Positive indicates moving forwards.
** If moving forwards, set pwm2 to 255 (stop), then move pwm1 down
** If moving backwards, set pwm1 to 255, then move pwm2 down
** Make sure that at least one of the pins is set to 255 at all times.
*/
void KoalaBear::drive (float target, uint8_t mtr)
{
	int pin1 = (mtr == MTRA) ? AIN1 : BIN1; //select the correct pins based on the motor
	int pin2 = (mtr == MTRA) ? AIN2 : BIN2;

    int direction = sign(target);
	int currpwm1 = 255;
	int currpwm2 = 255;

	// Determine how much to move the pins down from 255
	// Sanity check: If |target| == 1, move max speed --> pwm_difference == 255
	//				 If |target| == 0, stop moving	  --> pwm_difference == 0
    unsigned int pwm_difference = (target * direction) * 255 ; // A number between 0 and 255 inclusive (255 is stop; 0 is max speed);

	if (direction > 0) { // Moving forwards
		currpwm1 -= pwm_difference;
	} else if (direction < 0) { // Moving backwards
		currpwm2 -= pwm_difference;
	}

	analogWrite(pin1, currpwm1);
	analogWrite(pin2, currpwm2);
}
