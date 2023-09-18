# API Reference

The Student API is the set of functions available to students to communicate with their robots. The api can be split into two distinct class types. The Robot Class used for getting, setting, and running functions; input classes including the Gamepad Class and the Keyboard Class which are used to take student input to control their robots.

for more infomation refer to the Student API section of the [Runtime Wiki](https://github.com/pioneers/runtime/wiki)

#
[//]: # (find better word for code periods later)
* [Run Mode](#run-mode)
    * [`Teleop` Period](#teleop-period)
    * [`Auto` Period](#auto-period)
* [`Robot` Class](#robot-class)
    * [`Robot.get_value(device_id, param)`](#robotget_valuedevice_id-param)
    * [`Robot.set_value(device_id, param, value)`](#robotset_valuedevice_id-param-value)
    * [`Robot.run(function_name, args...)`](#robotrunfunction_name-args)
    * [`Robot.is_running(function_name)`](#robotis_runningfunction_name)
    * [`Robot.sleep(seconds)`](#robotsleepseconds)
* [Input Classes](#input-classes)
      * [`Gamepad.get_value(name_of_input)`](#gamepadget_valuename_of_input)
      * [`Keyboard.get_value(name_of_key)`](#keyboardget_valuename_of_key)

# Run Mode
The two run modes, `autonomous` and  `teleop` are where a majority of student code will be run. These functions are run in the Autonous period and Teleoperated period of a PiE compatition. Autonomous code restricts student's access to the `Keyboard` and `Gamepad` functions, forcing them to write code that will run without any input. The Teleoperated perod allows for control from both, giving students more freedom to control ther robot through any of the challenges presented in that phase. 



## `AUTO` Period
The autonomous or `AUTO` period is used anytime keyboard or controller input is restricted. Within this period you are encouraged to write code that will be able to run without any user input.

An example of this would be running a robot foward for a set duration within an `AUTO` period

```py
motor = "//INSERT MOTOR ID HERE//"

def autonomous_main():
  #keeps motor A running at max speed for 10 seconds
  Robot.set_value(motor, velocity_a, 1)
  Robot.sleep(10) #stops the execution of any other functions for a specified number of seconds
  Robot.set_value(motor, velocity_a, 0)
```
For more recources please refer to the [Software Hub Autonomous Guide](https://pioneers.berkeley.edu/competition/SoftwareHub/Teleop/)


## `TELEOP` Period
  The teleoperated or `TELEOP` period is used anytime remote input via a controller or keyboard is used. The `teleop_main()` function runs in a loop until the teleoperated period has completed.

  This code is most useful when a student would like to use controller or keyboard input to control their robot on the gamefield.

```py
motor = "//INSERT MOTOR ID HERE//"

def teleop_main():
    #sets the motor's velocity to max if the button A is pressed
    if Gamepad.get_value(button_a) == True: 
        Robot.set_value(motor, velocity_a, 1)
```

For more recources please refer to the [Software Hub Teleop Guide](https://pioneers.berkeley.edu/competition/SoftwareHub/Teleop/)
## `SETUP` Phase
  The `SETUP` phase prepares for either the `AUTO` or `TELEOP` function to be run. In this phase a student can set or get the value of any lowcar device (any device connected to the robot interactable using the PiE api).  

  This function is useful when a student would want to turn off the CURRENTLY BROKEN PiD control on a motor controller
```py
motor = "//INSERT MOTOR ID HERE//"

def teleop_setup(): #code segment run before the teleop_main() function
    Robot.set_value(motor, pid_enabled_a, False)
    Robot.set_value(motor, pid_enabled_b, False)
  
def autonomous_setup(): #code segment run before the autonomous_main() function
    Robot.set_value(motor, pid_enabled_a, False)
    Robot.set_value(motor, pid_enabled_b, False)
```

# `Robot` Class
The robot class holds all methods for interacting with various arduino devices connected to a student’s robot. These devices include servos, motors, and sensors. 

## `Robot.get_value(device_id, param)`
The get_value function returns the current value of a specified `param` of the device with the specified device_id. 

* `device_id`: the ID that specifies which PiE device will be read
* `param`: identifies which parameter on the specified PiE device will be read. Possible param values depend on the specified device. Find a list of params for each device on the device get page

The function is useful for checking the current state of devices. For example, getting the current state of the limit switch using its device_id and the param “switch0” will return the value True when pressed down and False if not.

```py
# first segment of code ran in the teleop process
limit_switch = "//INSERT SWITCH ID HERE//"

def teleop_setup():
    print("Tele-operated mode has started!")
    pass

def teleop_main():
    # example code for getting the value of a limit switch

    # first parameter is the limit switch's id
    # second parameter tells which switch to get the value from

    # in this case the method will retun True or False depending on if the switch is pressed down or not

    Robot.get_value(limit_switch, switch0)
    pass

```
For more examples and devices refer to the devices page in the reference


## `Robot.set_value(device_id, param, value)`
The `set_value` function sets a specified value to a device’s parameter

* `device_id`: the ID that specifies which PiE device will have its parameter set
* `param`: determines which parameter should be set. The parameters depend on the specified Lowcar device and can be found at INCLUDE LINK TO LOWCAR DEVICE PAGE
* `value` <int, bool, float> - the value to set the parameter to

This function is useful for setting the state of parts of your robot while driving. For example calling the set_value param `“velcoity_a”` with a KoalaBear device (motor controller) with a value of `1` results in the attached motor spinning forwards at full power


[//]: <> (MAKE CODE SEGMENTS COLLAPSABLE)
```py
# first segment of code ran in the teleop process
motor = "//INSERT MOTOR ID HERE//"

def teleop_setup():
    print("Tele-operated mode has started!")
    pass

def teleop_main():
    # example code for turning a motor foward at full speed

    # first parameter is the motor controller's id set as a varible
    # second parameter 'velocity_a' tells a motor controller which motor to drive
    # third parameter sets the value

    Robot.set_value(motor, velocity_a, 1)
    pass

```

## `Robot.run(function_name, args)`
The `Robot.run()` runs another function, passing the `args` fed into the function. The `function_name` is run in parallel to any other code run following the `Robot.run()` function.

* `function_name`: the name of a function in the student code which will be run simultaneously with the main loop

* `args`: this is a list of zero or more inputs, which will be passed to the function_name specified previously as arguments (inputs)

[//]: <> (I want to seperate the code example and this line of text. Not exactly sure how to do it so adding this comment to come back to it)
An example of this would if a student would want to run a process alongside the teleop_main loop. Using the Robot.sleep class to define the amount of time a motor should rotate for without stopping the teleop_main loop. 
​

[//]: <> (FULL CODE WITHOUT ANY REMOVED CODE SEGMENTS //REMOVE//)
```py
ARM_MOTOR = "INSERT MOTOR_ID HERE"
DRIVE_MOTOR = "INSERT MOTOR_ID HERE"


def arm_movement():
    # moves arm up for 1 second and then moves it down for 1 second
    # assumes arm is attached to motor A of MC "ARM_MOTOR"
    if Gamepad.get_value("button_a"):
        Robot.set_value(ARM_MOTOR, "velocity_a", 0.5)
        Robot.sleep(1)
        Robot.set_value(ARM_MOTOR, "velocity_a", -0.5)
        Robot.sleep(1)
​
def teleop_setup():
    # starts the arm_movement subroutine in parallel to the main thread
    Robot.run(arm_movement)
    pass
​
def teleop_main():
    # put your teleop code here and it will run along side the arm_movement thread
    if Gamepad.get_value("button_b"):
        Robot.set_value(DRIVE_MOTOR, "velocity_a", 1)

```


## `Robot.is_running(function_name)`
The `Robot.is_running()` function returns a boolean value (`True` or `False`) for whether or not the specified function is still running. 

`function_name`: the name of a function defined by the student. If run through `Robot.run()` it will be the same inputted `function_name`

An example usage of this would be to wait for a `Robot.run()` process to finish before allowing user input for another process.

```py

def robot_actions():
    #series of actions from the Robot.set_value().

def teleop_setup():
    pass

def teleop_main():
    if Gamepad.get_value(button_a) and not Robot.is_running(robot_actions):
        # if the button A is pressed down and robot_actions are not running.
        # Then the robot will be able to run robot_actions again. Will not run if the button A is not pressed or
        # if robot_actions is running.
        Robot.run(robot_actions)


```



## `Robot.sleep(seconds)`
Pauses the execution of the current function for the specified number of `seconds`.

`seconds`: the number of `seconds` to pause the execution of the current function for.
It should be emphasized that calling `Robot.sleep` in one function does not cause any other function that is being run by `Robot.run()` or the main loop function to pause. Only the instance of the function that is executing the call to `Robot.sleep` will pause execution. It is highly recommended to not use this function in the setup functions or main loop functions--only in functions executed by `Robot.run()`.

a great place to use `Robot.sleep()` would be to make a robot go to a specific spot using set motor velocities and ammount of time each function should run. 
[//]: <> (could go further and say that `Robot.sleep()` combined with encoder ticks per revolution could allow you to specify a distance your robot could go)

```py
MOTOR_ID = "INSERT MOTOR_ID HERE"

def autonomous_setup():
    print("Autonomous mode has started!")
    robot.run(autonomous_actions) #runs the autonomous_actions function in parallel to autonomous main


def autonomous_main():
    pass

def autonomous_actions():
    print("Action 1")   # action one sets the motor velocites to 1
    Robot.set_value(MOTOR_ID, "velocity_b", 1.0)
    Robot.set_value(MOTOR_ID, "velocity_a", 1.0)
    Robot.sleep(1.0)    # holds the function for one second before running the next lines
    print("Action 2")   # the following code sets the motor velocities to 0
    Robot.set_value(KOALA_BEAR, "velocity_b", 0)
    Robot.set_value(KOALA_BEAR, "velocity_a", 0)

```

# Input Classes
The input classes are both of the input classes that can only be run durring the teleoperated game phase. These two classes allow a student to recieve a booliean value for whether or not a button is pressed down or not. Most students will use these classes to 

## `Gamepad.get_value(input)`
The `Gamepad` class recieves input for when there is a change to a controller. This class is only usable durring the teleop phase.

`input`: identifies which button or joystick will be returned. This function is useful for checking the state of a button or joystick

For example, if you wanted to print `"hello world"` when the `"button_a"` is pressed and false when it isn't you would use the function as a condition in an if statement

```py
# segment of code will print "hello world" into the console when button_a is pressed
def teleop_main():
    if Gamepad.get_value("button_a"):
        print("hello world")
```
This function is essential for controlling your robot with the gamepad. 

The possible button inputs are:
* "button_a"
* "button_b"
* "button_x"
* "button_y"
* "l_bumper"
* "r_bumper"
* "l_trigger"
* "r_trigger"
* "button_back"
* "button_start"
* "l_stick"
* "r_stick"
* "dpad_up"
* "dpad_down"
* "dpad_left"
* "dpad_right"
* "button_xbox"

The possible joystick inputs are:
* "joystick_left_x"
* "joystick_left_y"
* "joystick_right_x"
* "joystick_right_y"

Note that the joysticks function differently from the button inputs. Rather then returning a Boolean `[True or False]`, they return a floating point value ranging from `-1.0 `and` 1.0` (inclusive)

## `Keyboard.get_value(input)`
The `Keyboard` class allows a student to recieve keyboard input. Like the `Gamepad` class, it is only usable durring the teleop game phase.

`input`: identifies which key is being read. When pressed the `get_value` will return a `True` boolean and `False` when not.

possible keyboard inputs are:
* The letters on the keyboard, lowercase and no spaces `"a"`-`"z"`
* The numbers on the keyboard, no spaces `"0"`-`"9"`
* The punctuation keys `","`, `"."`,  `"/"`,  `";"`,  `"'"`,  `"["`,  `"]"`
* The four arrow keys `"left_arrow"`, `"right_arrow"`, `"up_arrow"`, `"down_arrow"`

For example, if you wanted to print `"hello world"` when the `"w"` is pressed and false when it isn't you would use the function as a condition in an if statement
 
```py
# segment of code will print "hello world" into the console when the w key is pressed
def teleop_main():
    if Keyboard.get_value("w"):
        print("hello world")


```