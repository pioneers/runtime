import time
import math

SIMPLE_DEV = '62_4660'  # 4660 = 0x1234
i = 0

def print_if_button_a():
    while True:
        if Gamepad.get_value('button_a'):
            print("a button pressed!")
        else:
            print("a button not pressed!")
        Robot.sleep(1)

def modify_my_int():
    while True:
        if Gamepad.get_value('dpad_down'):
            Robot.set_value(SIMPLE_DEV, "MY_INT", Robot.get_value(SIMPLE_DEV, "MY_INT") + 1)
        else:
            Robot.set_value(SIMPLE_DEV, "MY_INT", Robot.get_value(SIMPLE_DEV, "MY_INT") - 1)
        Robot.sleep(1)

def teleop():
    global i
    if i < 3 and Gamepad.get_value('joystick_left_x') != 0.0:
        print("Left joystick moved in x direction!")
        i += 1
