# Tests Issue 212
# Writes to SimpleTestDevice's "MY_INT"
# Teleop Setup: Writes
# Teleop Main: Writes nonzero only when a certain key is pressed
# This code may seem contrived, but is useful to know which Robot.set_value() actually executes

DEVICE = "62_1"
PARAM = "MY_INT"

def autonomous_setup():
    pass

def autonomous_main():
    pass

def teleop_setup():
    Robot.set_value(DEVICE, PARAM, 999)

def teleop_main():
    if Keyboard.get_value('w'):
        Robot.set_value(DEVICE, PARAM, 1000)
    