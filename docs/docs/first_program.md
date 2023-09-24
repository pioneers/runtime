# Getting Started

Python is a interperited, object-oriented, high-level programming languge. All robots in PiE are coded in Python 3 using a Application User Interface (API) known as the the student API. This API includes essental methods such as `Robot.get_value()`, `Robot.set_value()`, `keyboard.get_value()`, and `Gamepad.get_value()`, which you'll frequently use as a student.

This section is intended for individuals with a basic understanding of Python coding. If you're new to Python, we recommend startiing with [The Python Tutorial](https://docs.python.org/3/tutorial/index.html) first to familiarize yourself with the language. 
## Teleoperated And Autonomous

The student rulebook outlines two distinct phases: the teloperated phase and the Autonomous phase. During the Autonomous phase, you are restricted to utilizing only the `Robot` class and it's methods. In contrast to the Teleoperated mode allowing use for all classes and thus the ability to control the robot in real time.

# Autonomous Mode

The autonomous mode is the first game phase you will encounter. In this mode, you will be required to navigate a task without the use of any input classes: `gamepad` or `keyboard`. Autonomous actions defined in the `def autonomous_main()` will only run once from top to bottom. In contrast, the Teleoperated phase gives you unrestricted access to all of the Student API classes. This means you're allowed to use input from both the `Keyboard` and `Gamepad` classes. Also keep in mind the function `def teleop_main()` will run in a continous loop. This is where you can use decision making statements like `if`, `elif`, and/or `else` to control any of the PiE devices.

## Autonomous Setup

The autonomous setup phase is what happens before the autonomous main function. Within the function `def autonomous_setup()` any code ran within this function will be done before running the `def autonomous_main()` function. The best use for this would be to set the value of a device before the autonomous main function.

For instance, if you wanted your motors to drive in the oppisite direction as your set velocity you could do something like:

```py
motor_id = "YOUR MOTOR ID HERE" # varible that holds the motor_id


def autonomous_setup(): # setup function
    Robot.set_value(motor_id, "invert_a", True) # inverts the direction of the device with a coresponding motor_id
```

## Autonomous Main

Autonomous Main is the function run after `autonomous_setup`. In the function `autonomous_main()` you are allowed to write a sequence of functions that will be run durring this period. While running, main function will sequentially run through the list of operations until completed. This could be useful if you want a sequence of python functions to be run one after another without any delay's in the sequence.

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
    Robot.sleep(10) # stops for 10 seconds
    Robot.set_value(motor_id, "velocity_a", 0.5)
    Robot.sleep(0.5) # stops for 0.5 seconds
    Robot.set_value(motor_id, "velocity_a", 0)
    # sequence of commands sets the speed of motor A to max speed for ten seconds, then sets the speed to 1/2 of that speed and finally stops the motor

def autonomous_setup(): # runs before autonomous_main()
# note that you can still call Robot.run() here

def autonomous_main():
    # where the autonomous actions are ran
    Robot.run(autonomous_actions) # runs the function autonomous actions on a seperate thread
```

# Teleoperated Mode

Following the Autonomous mode, the Teleoperated Mode happens after the Autonomous phase ends. Within this phase, you are granted the ability to use input from both the `Keyboard` and `Gamepad` classes, allowing you to directly your robot. To use Teleoperated functions, you can use the `def teleop_main()` and `def teleop_setup()` methods, with the former executed after the latter to ensure proper initialization and seamless functionality.



## Teleop Setup

Just as with the autonomous setup, the Teleop setup and Autonomous setup serve the same purpose. The `def teleop_setup():` function is best used to preconfigure any necessary values or settings prior to executing the `teleop_main` loop. To call the Teleop setup use the function `def telop_setup():`. This function is used to prepare any values before running the main telop loop. 

A example usage of this would be to turn off the pid on the motor controllers. To do this do:

```py
motor_id = "YOUR MOTOR ID HERE" # varible that holds the motor_id

def teleop_setup(): # setup function
    Robot.set_value(motor_id, "pid_enabled_a", False) # disables pid on motor 1
    Robot.set_value(motor_id, - "pid_enabled_b", False) # disables pid motor 2
```


## Teleop Main

Just as with the `autonomous_main()` the `teleop_main()` function serves as the primary place to execute all of your code. The teleop function is the only function that allows for student input from both the Keyboard and Gamepad classes. Using the teleop main loop, you can create if statements to read your input and thus change the values of any attached robot devices. 

To best take advantage of the teleop_main loop, you can create a control structure using `if`, `elif`, and `else` statements. In this example a Robot is responding to when a keyboard w key is pressed.

```py
motor_id = "YOUR MOTOR ID HERE" # varible that holds the motor_id

def teleop_setup(): # setup function
    pass


def teleop_main():
    if(Keyboard.get_value("w") == True):
        Robot.set_value(motor_id, "velocity_a", 1.0) # sets the value of motor 1 to 100% power if w is pressed
    else:
        Robot.set_value(motor_id, "velocity_a", 0.0) # sets the value of motor 1 to 0% power when w is not pressed
```


# Example Code
Finally this example program combines all of the other elements used in the code. You can expect your code to look similar to this.

```py
left_motor = "YOUR MOTOR ID HERE"
right_motor = "YOUR MOTOR ID HERE"


#--------------- Autonomous Mode ---------------#
def autonomous_setup(): # determines what will be done before  `def_autonomous_main():` runs
    print("Autonomous mode has started!")

def autonomous_main(): # where you put a sequence of actions
    Robot.run(autonomous_actions)


#--------------- Teleoperated Mode ---------------#
def teleop_setup():
    Robot.run(teleop_input)
    print("Tele-operated mode has started!")

def teleop_main():
    if(Keyboard.get_value("w") == True):
        Robot.set_value(motor_id, "velocity_a", 1.0) # sets the value of motor 1 to 100% power if w is pressed
    else:
        Robot.set_value(motor_id, "velocity_a", 0.0) # sets the value of motor 1 to 0% power when w is not pressed

#--------------- Extra Functions ---------------#
async def autonomous_actions(): # a seperate function that runs
    print("Autonomous action sequence started")
    await Actions.sleep(1.0)
    print("1 second has passed in autonomous mode")

async def teleop_input():
    if(Keyboard.get_value("d") == True):
        Robot.set_value(motor_id, "velocity_b", 1.0) # sets the value of motor 2 to 100% power if w is pressed
    else:
        Robot.set_value(motor_id, "velocity_b", 0.0) # sets the value of motor 3 to 0% power when w is not pressed

```
