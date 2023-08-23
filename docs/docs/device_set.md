# `Robot.set_value(device, param)` for Lowcar Devices


## Servo Controller
![Servo Controller](https://raw.githubusercontent.com/pioneers/runtime-docs/master/device-pics/ServoControl.JPG)
sets a value associated with the `device` and `param` specified.

The device being specified is a Servo.

`param` for a Servo:
* ``"servo0"``
* ``"servo1"``

Changing values for the `servo` spins the servo to an angle based on the value entered. The values -1 and 1 each refer to the maximum position of the servo in one direction. For example, if we described our minimum position to be 0°, and our maximum to be 180°, then 1 would set the servo to 180°, -1 would set the servo to 0°, and -0.5 would set it to be 45°.


**Sample Usage:**
```py
#sets the current angle of servo0 to 180° 
Robot.set_value(servo_id, "servo0", 1)
```
## KoalaBear
![KoalaBear](https://raw.githubusercontent.com/pioneers/runtime-docs/master/device-pics/KoalaBear.JPG)
sets a value associated with the `device` and `param` specified.

The device being specified is a YogiBear.

parameters for a YogiBear:

Motor A
* `"velocity_a"`
* `"deadband_a"`
* `"invert_a"`
* `"pid_enabled_a"`
* `"pid_kp_a"`
* `"pid_ki_a"`
* `"pid_kd_a"`
* `"enc_a"`

Motor B
* `"velocity_b"`
* `"deadband_b"`
* `"invert_b"`
* `"pid_enabled_b"`
* `"pid_kp_b"`
* `"pid_ki_b"`
* `"pid_kd_b"`
* `"enc_b"`


Motor control is handled through  changing the values of `"velocity_a"` and `"deadband_a"`.
* `"velocity"` which sets a float from -1 to 1 which describes the direction the motor is turning and it's power.
* `"deadband"` which sets the threshold that velocity must pass before a change is made to `"velocity"`

The encoder's value can be set using the `"enc"` parameter. Setting the `"enc"` count allows for someone to change the number of ticks recorded from the encoder. 

PiD Control includes:
* `"pid_kp"`, `"pid_ki"`, and `"pid_kd"` parameters which set the proportional, integral, and derivative coefficients on the motor controller. Each of these values have to be above zero.
* `"pid_enabled"` parameter sets whether or not pid is enabled for a motor.


Sample Usage:
```py
#sets the current speed of Motor A to max speed
Robot.set_value(motor_id, "velocity_a", 1)
```