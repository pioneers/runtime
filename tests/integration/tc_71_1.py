from tester import *

# setup
start_test("sanity")
start_shm()
start_net_handler()

# poke the system
send_run_mode(SHEPHERD, AUTO)
print_shm()
send_start_pos(SHEPHERD, RIGHT)
print_shm()

#  stop the system
stop_net_handler()
stop_shm()
end_test()

output1 = """Changed devices: 00000000000000000000000000000000
Changed params:
Requested devices: 00000000000000000000000000000000
Requested params:
Current Robot Description:
        RUN_MODE = AUTO
        DAWN = CONNECTED
        SHEPHERD = CONNECTED
        GAMEPAD = DISCONNECTED
        START_POS = LEFT 

Current Gamepad State:
        Pushed Buttons:
        Joystick Positions:
                joystick_left_x = 0.000000
                joystick_left_y = 0.000000
                joystick_right_x = 0.000000
                joystick_right_y = 0.000000"""

output2 = """Changed devices: 00000000000000000000000000000000
Changed params:
Requested devices: 00000000000000000000000000000000
Requested params:
Current Robot Description:
        RUN_MODE = AUTO
        DAWN = CONNECTED
        SHEPHERD = CONNECTED
        GAMEPAD = DISCONNECTED
        START_POS = RIGHT 

Current Gamepad State:
        Pushed Buttons:
        Joystick Positions:
                joystick_left_x = 0.000000
                joystick_left_y = 0.000000
                joystick_right_x = 0.000000
                joystick_right_y = 0.000000"""

assert_output(output1, remaining=True)
assert_output(output2, remaining=True)
