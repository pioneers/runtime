# `Robot.get_value(device, param)` for Lowcar Devices

## Limit Switch
![Limit switch](https://raw.githubusercontent.com/pioneers/runtime-docs/master/device-pics/LimitSwitch.JPG)

Returns a value from a specified `device_id` and `param`

`param` for a Limit Switch
* `"switch0"`
* `"switch1"`
* `"switch2"`

The parameters for a Limit Switch describe which of the three switches is being read. The boolean value will be `True` if the specified switch is being pressed and `False` if it is not.

**Sample Usage:**
```py
#returns a boolean for whether or not switch0 is pressed
Robot.get_value(limit_switch_id, "switch0")
```


## Line Follower
![Line Follower](https://raw.githubusercontent.com/pioneers/runtime-docs/master/device-pics/LineFollower.JPG)
Returns a value associated with the `device_id` and `param` specified.

The device being specified is a Line Follower.

parameters for a Line Follower:
* `"left"`
* `"center"`
* `"right"`

The `param` for a Line Follower describe how much light is being reflected into each sensor. It returns a value between 0 and 1 where a lower value means less light and the sensor is farther off of the reflective tape.

**Sample Usage:**
```py
#returns how much light is seen from the center sensor on the line follower
Robot.get_value(line_follower_id, "center")
```

## Servo Controller
![Servo Controller](https://raw.githubusercontent.com/pioneers/runtime-docs/master/device-pics/ServoControl.JPG)
Returns a value associated with the `device` and `param` specified.

The device being specified is a Servo.

`param` for a Servo:
* ``"servo0"``
* ``"servo1"``

The parameters for a Servo describes what angle the servo has turned to. It returns a Float from -1 to 1 where both -1 and 1 represent the two end positions for the servo.

**Sample Usage:**
```py
#returns the current angle of servo0 
Robot.get_value(servo_id, "servo0")
```
## KoalaBear
![KoalaBear](https://raw.githubusercontent.com/pioneers/runtime-docs/master/device-pics/KoalaBear.JPG)
Returns a value associated with the `device` and `param` specified.

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


The parameters for a KoalaBear can be split into 3 categories:

Motor Control includes:
* `"velocity"` which returns a float from -1 to 1 which describes the direction the motor is turning and it's power.
* `"deadband"` which returns the threshold that velocity must pass before a change is made to `"velocity"`

Encoder Control includes:
* `"enc"` parameters which return information about the position and velocity of the robot. Position is returned as an integer that represents the number of ticks of the encoder. There are 46 per revolution of the encoder. 

PiD Control includes:
* `"pid_kp"`, `"pid_ki"`, and `"pid_kd"` parameters which return the proportional, integral, and derivative coefficients on the motor controller. 
* `"pid_enabled"` parameter returns whether or not pid is enabled for a motor.


Sample Usage:
```py
#returns the current speed of Motor A as a value from -1 to 1
Robot.get_value(motor_id, "velocity_a")
```