PolarBear = '12_2541551405711093506' # flashed with -t
Switch = '0_2882400000' # ABCDEF

prev_switch0 = True
prev_switch1 = True
prev_switch2 = True

def teleop_setup():
    print("in teleop setup!")

def teleop_main():
    global prev_switch0
    global prev_switch1
    global prev_switch2

    if (Gamepad.get_value('button_a')):
        #print("here1")
        Robot.set_value(PolarBear, 'duty_cycle', 0.5)
    elif (Gamepad.get_value('button_b')):
        #print("here2")
        Robot.set_value(PolarBear, 'duty_cycle', -0.5)
    else:
        #print("here3")
        Robot.set_value(PolarBear, 'duty_cycle', 0.0)
    # print(Robot.get_value(PolarBear, 'duty_cycle'))
    Robot.sleep(0.05)

    switch0 = Robot.get_value(Switch, 'switch0')
    switch1 = Robot.get_value(Switch, 'switch1')
    switch2 = Robot.get_value(Switch, 'switch2')

    if switch0 != prev_switch0:
        print(f"switch0 switched to {switch0}")
        prev_switch0 = switch0
    if switch1 != prev_switch1:
        print(f"switch1 switched to {switch1}")
        prev_switch1 = switch1
    if switch2 != prev_switch2:
        print(f"switch2 switched to {switch2}")
        prev_switch2 = switch2
