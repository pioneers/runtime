# Getting Started

Python is a interperited, object-oriented, high-level programming languge. All robots in PiE are coded in Python 3 using a Application User Interface (API) called the student API. This includes all of the front facing classes you will use as a student including `Robot.get_value()`, `Robot.set_value()`, `keyboard.get_value()`, and `gamepad.get_value()`. This section overviews the basic organization of a student robot as well as ways you can debug and trobbleshoot any code issues.


## Teleoperated And Autonomous
The teleoperated and Autonomous phases are the two phases outlined in the student rulebook. In the Autonomous phase, you are only allowed to use the `Robot` class from the Student API. Each action must be planned out before the Autonomous phase using a combination of timers and encoder steps. 

The Student API includes a couple of functions built in.

# Autonomous Mode
The autonomous mode is the first game phase you will encounter. In this mode, you will be required to navigate a task without the use of any input classes: `gamepad` or `keyboard`. Autonomous actions defined in the `def autonomous_main()` will only run once from top to bottom. In contrast, the Teleoperated phase gives you unrestricted access to all of the Student API classes. This means you're allowed to use input from both the `Keyboard` and  `Gamepad` classes. Also keep in mind the function `def teleop_main()` will run in a continous loop. This is where you can use decision making statements like `if`, `elif`, and/or `else` to control any of the PiE devices.

## Autonomous Setup

This function is run before the Autonomous main function. This is where you could set the values of different attached devices. For instance, if you wanted your motors to drive in the oppisite direction as your set velocity you could do something like:

```py
motor_id = "YOUR MOTOR ID HERE" # varible that holds the motor_id


def autonomous_setup(): # setup function
    Robot.set_value(motor_id, "invert_a", True) # inverts the direction of the device with a coresponding motor_id
```

## Autonomous Main
Autonomous Main is the game phase that's ran after setting up. In the function `autonomous_main` you are allowed to write a sequence of functions that will be run durring this period. While running, main function will sequentially run through the list of operations until completed. This could be useful if you want a sequence of python functions to be run one after another without any delay's in the sequence.


This sample code uses the `autonomous_main():` function to set the value of a motor to full speed
```py
motor_id = "YOUR MOTOR ID HERE" # varible that holds the motor_id

def autonomous_setup(): # setup function runs before autonomous main
    pass

def autonomous_main():
Robot.set_value(motor_id, "velocity_a", 1) # sets the motor speed to the max speed
```

The `autonomous_main` function and `autonomous_setup` aren't very useful alone. However combined with the `Robot.run()` allows for further control over the timings of the code sequence. `Robot.run()` allows someone to run a process on a seperate thread (runs the code at the same time as a main loop). This means a student will be able to use `Robot.sleep(seconds)` to wait before running the next function call. 

do note that although it's not required, you're recommended to assign the function type `async` for any function being run within the `Robot.run()` function. This will define the function as a coroutine and make it easier to seperate from any of your other functions.

```py
motor_id = "YOUR MOTOR ID HERE" # varible that holds the motor_id

async def autonomous_actions(): # a custom asynchronous function with a sequence of actions
    Robot.set_value(motor_id, "velocity_a", 1)
    Robot.sleep(10)  # stops for 10 seconds
    Robot.set_value(motor_id, "velocity_a", 0.5)
    Robot.sleep(0.5) # stops for 0.5 seconds
    Robot.set_value(motor_id, "velocity_a", 0)

# sequence of commands sets the speed of motor A to max speed for ten seconds, then sets the speed to 1/2 of that speed and finally stops the motor



def autonomous_setup(): # runs before autonomous_main()
    Robot.run(autonomous_actions) # runs the function autonomous actions on a seperate thread

def autonomous_main():
    # where the autonomous actions are ran
```



# Teleoperated Mode


## `def teleop_main()`



## Template Code
This example code is how your program could be organized. Each 


```py

left_motor = "YOUR MOTOR ID HERE"
right_motor = "YOUR MOTOR ID HERE"
def autonomous_setup(): # determines what will be done before  `def_autonomous_main():` runs
    print("Autonomous mode has started!")
    Robot.run(autonomous_actions)

def autonomous_main(): # where you put a sequence of actions
    pass

async def autonomous_actions(): # a seperate function that runs
    print("Autonomous action sequence started")
    await Actions.sleep(1.0)
    print("1 second has passed in autonomous mode")

def teleop_setup():
    print("Tele-operated mode has started!")

def teleop_main():
    if Gamepad.get_value("joystick_right_y") > 0.5:
        Robot.set_value(left_motor, "duty_cycle", -0.5)
        Robot.set_value(right_motor, "duty_cycle", -0.5)
    else:
        Robot.set_value(left_motor, "duty_cycle", 0)
        Robot.set_value(right_motor, "duty_cycle", 0)

```