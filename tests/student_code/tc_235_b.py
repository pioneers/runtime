# Tests Issue 235
# Attempts to write to GeneralTestDevice for a parameter that is not writeable
# Teleop Setup: No writing
# Teleop Main: Writes false only when a certain key is pressed

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
        Robot.set_value(DEVICE, PARAM, false)
    