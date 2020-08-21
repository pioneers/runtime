import time

time_dev = '60_123'

def autonomous_setup():
    pass

def autonomous_main():
    pass

def teleop_setup():
    pass

def teleop_main():
    if Gamepad.get_value('a_button'):
        Robot.set_value(time_dev, "GET_TIME", True)
