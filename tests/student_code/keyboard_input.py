
simple_device = "62_20"

def autonomous():
    pass

def teleop():
    while True:
        if Keyboard.get_value('a'):
            Robot.set_value(simple_device, "MY_INT", 123454321)
        elif Keyboard.get_value('y'):
            print(Robot.get_value(simple_device, "MY_INT"))
            Robot.sleep(.45)
