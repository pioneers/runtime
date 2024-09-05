import time

# 0x0123456789ABCDEF == 0d81985529216486895
GeneralTestDevice = '63_81985529216486895'

def constant_write():
    """
    Constantly increments RED_INT by 2, ORANGE_FLOAT by 3.14, and flips YELLOW_BOOL
    """
    int_val = 1
    float_val = 1.0
    bool_val = True
    while True:
        time.sleep(2)
        Robot.set_value(GeneralTestDevice, "RED_INT", int_val)
        Robot.set_value(GeneralTestDevice, "ORANGE_FLOAT", float_val)
        Robot.set_value(GeneralTestDevice, "YELLOW_BOOL", bool_val)
        int_val += 2;
        float_val += 3.14
        bool_val = not bool_val

def autonomous():
    Robot.run(constant_write)

def teleop():
    pass
