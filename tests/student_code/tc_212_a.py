# Tests Issue 212
# Writes to SimpleTestDevice's "MY_INT"
# Teleop Setup: No writing
# Teleop Main: Always writes

DEVICE = "62_1"
PARAM = "MY_INT"

def autonomous():
    pass

def teleop():
    Robot.set_value(DEVICE, PARAM, 999)
    
