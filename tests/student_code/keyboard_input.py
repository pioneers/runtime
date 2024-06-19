'''
simple_device = "62_20"

def autonomous_setup():
    pass

def autonomous_main():
    pass

def teleop_setup():
    pass

def teleop_main():
    if Keyboard.get_value('a'):
        Robot.set_value(simple_device, "MY_INT", 123454321)
    elif Keyboard.get_value('y'):
        print(Robot.get_value(simple_device, "MY_INT"))
        Robot.sleep(.45)
'''

simple_device = "62_20"

def autonomous():
    while True:
        pass

def teleop():
    while True:
        if Keyboard.get_value('a'):
            Robot.set_value(simple_device, "MY_INT", 123454321)
        elif Keyboard.get_value('y'):
            print(Robot.get_value(simple_device, "MY_INT"))
            Robot.sleep(0.45)
