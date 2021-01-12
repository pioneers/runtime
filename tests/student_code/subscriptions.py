# Student code that tests executor's ability to parse parameter subscriptions
# Executor should be able to 
#    - match all calls to Robot.get_value()
#    - evaluate intermediate global variables (we're telling students not to use local variables for names)
#    - handle 0 or more white space
#    - handle excessive parentheses
#
# Expected subscriptions:
#    62_2: ['FLIP_FLOP', 'DECREASING', 'INCREASING]
#    63_1: ['EXP_ONE_PT_ONE', 'INCREASING_ODD', 'DECREASING_ODD']

SIMPLE = '62_2'
GENERAL = '63_1'
# Intermediate variables to test evaluating variables
SIMPLE_1 = SIMPLE
SIMPLE_2 = SIMPLE_1
# Parameter name variable
parameter = "INCREASING_ODD"
# Using a variable in studentapi.pyx (Try also "code_file")
param_name = "DECREASING_ODD"

def autonomous_setup():
    # Try strange parenthesis combinations and single/double quotes
    if ((Robot.get_value(SIMPLE_1, 'FLIP_FLOP')) or Robot.get_value(SIMPLE_2, "DECREASING")):
        # Try no spacing between args
        param_val = Robot.get_value(SIMPLE,"INCREASING")

        # Try absurd spacing between args and params with multiple underscores
        param_val = Robot.get_value(  GENERAL ,    'EXP_ONE_PT_ONE' )
         
        # Try a variable as the parameter name
        param_val = Robot.get_value(GENERAL, parameter )

        # Try a studentapi variable as the parameter name
        param_val = Robot.get_value(GENERAL, param_name )

        # Robot.set_value() shouldn't subscribe
        Robot.set_value(  GENERAL ,    'RED_INT' , 1337)   

        # Robot.log() shouldn't subscribe
        Robot.log("Pi", 3.1415)

def autonomous_main():
    pass

def teleop_setup():
    pass

def teleop_main():
    pass
