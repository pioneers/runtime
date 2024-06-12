# Tests Issue 212
# Writes to SimpleTestDevice's "MY_INT"
# Teleop Setup: No writing
# Teleop Main: Writes nonzero only when a certain key is pressed; Otherwise writes 111

DEVICE = "62_1"
PARAM = "MY_INT"

def autonomous():
    pass

def teleop():
    while True:
        if Keyboard.get_value('w'):
            Robot.set_value(DEVICE, PARAM, 999)
        else:
            Robot.set_value(DEVICE, PARAM, 111)