import time

time_dev = '60_123'

def autonomous():
    pass

def teleop():
    while True:
        if Gamepad.get_value('button_a'):
            Robot.set_value(time_dev, "GET_TIME", True)
        Robot.sleep(0.1)
