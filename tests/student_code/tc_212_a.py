# Tests Issue 212
# Writes to SimpleTestDevice's "MY_INT"
# Teleop Setup: No writing
# Teleop Main: Always writes

DEVICE = "62_1"
PARAM = "MY_INT"

def autonomous_setup():
    pass

def autonomous_main():
    pass

def teleop_setup():
    pass

def teleop_main():
    Robot.set_value(DEVICE, PARAM, 999)
    
