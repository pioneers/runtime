# Tests Issue 212
# Writes to SimpleTestDevice's "MY_INT"
# Teleop Setup: Writes
# Teleop Main: Writes nonzero only when a certain key is pressed
# This code may seem contrived, but is useful to know which Robot.set_value() actually executes

DEVICE = "62_1"
PARAM = "MY_INT"

def autonomous():
    pass

def teleop():
    Robot.set_value(DEVICE, PARAM, 999)
    while True:
        if Keyboard.get_value('w'):
            Robot.set_value(DEVICE, PARAM, 1000)
    