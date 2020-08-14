from tester import *

# setup
start_test("UDP no devices connected")
start_shm()
start_net_handler()

# poke system
buttons = (1 << BUTTON_A) | (1 << L_TRIGGER) | (1 << DPAD_DOWN)
joystick_vals = [-0.1, 0.0, 0.1, 0.99]
send_gamepad_state(buttons, joystick_vals)
print_shm()
print_next_dev_data()

# stop all processes
stop_net_handler()
stop_shm()
end_test()

output1 = """Current Robot Description:
        RUN_MODE = IDLE
        DAWN = CONNECTED
        SHEPHERD = CONNECTED
        GAMEPAD = CONNECTED
        START_POS = LEFT

Current Gamepad State:
        Pushed Buttons:
                button_a
                l_trigger
                dpad_down
        Joystick Positions:
                joystick_left_x = -0.100000
                joystick_left_y = 0.000000
                joystick_right_x = 0.100000
                joystick_right_y = 0.990000"""

output2 = "Device No. 0:   type = CustomData, uid = 0, itype = 32"

# # check outputs
assert_output(output1, True)
assert_output(output2, True)
