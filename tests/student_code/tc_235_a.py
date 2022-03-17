# Tests Issue 235
# Attempts to read from GeneralTestDevice for a parameter that is not readable
# Teleop Setup: No writing
# Teleop Main: Reads the param only when a certain key is pressed

DEVICE = "63_1"
PARAM = "ALWAYS_TRUE"

def autonomous_setup():
    pass

def autonomous_main():
    pass

def teleop_setup():
    pass

def teleop_main():
    if Keyboard.get_value('w'):
        Robot.get_value(DEVICE, PARAM)
    