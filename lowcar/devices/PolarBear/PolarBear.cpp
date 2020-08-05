#include "PolarBear.h"

// enumerate the parameters
typedef enum {
    DUTY_CYCLE = 0,
    MOTOR_CURRENT = 1,
    DEADBAND = 2
} param;

PolarBear::PolarBear () : Device(DeviceType::POLAR_BEAR, 2) {
    this->duty_cycle = 0.0;
    this->deadband = 0.05;
    
    this->dpwm_dt = 255 / 200000;
}

size_t PolarBear::device_read(uint8_t param, uint8_t *data_buf) {
    float* float_buf = (float *) data_buf;

    switch (param) {
        case DUTY_CYCLE:
            float_buf[0] = this->duty_cycle;
            return sizeof(this->duty_cycle);
        case MOTOR_CURRENT:
            float_buf[0] = (analogRead(FEEDBACK) / 0.0024);
            return sizeof(float);
        case DEADBAND:
            float_buf[0] = this->deadband;
            return sizeof(this->deadband);
        default:
            return 0;
    }
    return 0;
}

size_t PolarBear::device_write(uint8_t param, uint8_t *data_buf) {
    float *float_buf = (float *) data_buf;
    switch (param) {
        case DUTY_CYCLE:
            // Change duty_cycle only if abs(input) is greater than deadband
            if (!(float_buf[0] >= -1.0 * this->deadband && float_buf[0] <= this->deadband)) {
                this->duty_cycle = float_buf[0];
            } else {
                this->duty_cycle = 0.0;
            }
            return sizeof(this->duty_cycle);
        case MOTOR_CURRENT:
            break;
        case DEADBAND:
            this->deadband = float_buf[0];
            return sizeof(this->deadband);
        default:
            return 0;
    }
    return 0;
}

void PolarBear::device_enable() {
    // Start LEDs
    setup_LEDs();

    pinMode(FEEDBACK,INPUT);
    pinMode(PWM1, OUTPUT);
    pinMode(PWM2, OUTPUT);
}

void PolarBear::device_disable() {
    // Stop motors
    this->device_write(DUTY_CYCLE, 0);
}

void PolarBear::device_actions() {
    ctrl_LEDs(this->duty_cycle, this->deadband);
    drive(this->duty_cycle);
}

/* Given a value between -1 and 1 inclusive,
 * analogWrite to the appropriate pins to accelerate/decelerate
 * Negative indicates moving backwards; Positive indicates moving forwards.
 * If moving forwards, set pwm1 to 255 (stop), then move pwm2 down
 * If moving backwards, set pwm2 to 255, then move pwm1 down
 * Make sure that at least one of the pins is set to 255 at all times.
 */
void PolarBear::drive(float target) {
    float direction = (target > 0.0) ? 1.0 : -1.0; // if target == 0.0, direction will be -1 ("backwards")
    int currpwm1 = 255;
    int currpwm2 = 255;

    // Determine how much to move the pins down from 255
    // Sanity check: If |target| == 1, move max speed --> pwm_difference == 255
    //               If |target| == 0, stop moving    --> pwm_difference == 0
    int pwm_difference = (int)(target * direction * 255.0); // A number between 0 and 255 inclusive (255 is stop; 0 is max speed);
    
    // We don't catch direction 0.0 because it will be either 1.0 or -1.0.
    // If stopped (target == 0.0) then 255 will be written to both drive pins
    if (direction > 0.0) { // Moving forwards
        currpwm1 -= pwm_difference;
    } else if (direction < 0.0) { // Moving backwards
        currpwm2 -= pwm_difference;
    }
    
    analogWrite(PWM1, currpwm1);
    analogWrite(PWM2, currpwm2);
    delayMicroseconds(1 / this->dpwm_dt); // About 784
}
