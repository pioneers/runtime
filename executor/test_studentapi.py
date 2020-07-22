import studentapi

# Add tests for API
POLARBEAR = '12_2604648'

def test_api():
    studentapi._init()
    robot = studentapi.Robot()
    val = robot.get_value(POLARBEAR, 'enc_pos')
    print(f"Read value: {val}")
    print(robot.get_value(POLARBEAR, 'duty_cycle'))

    robot.set_value(POLARBEAR, 'pid_vel_setpoint', 19.023)
    val = robot.get_value(POLARBEAR, 'pid_vel_setpoint')
    print(f"Old value: {val}")
    robot.set_value(POLARBEAR, 'pid_vel_setpoint', 10.0)
    val = robot.get_value(POLARBEAR, 'pid_vel_setpoint')
    print(f"New value: {val}")

    gamepad = studentapi.Gamepad()
    try:
        print("Button A:", gamepad.get_value('button_a'))
        print("R bumper:", gamepad.get_value('r_bumper'))
        print("R X joystick:", gamepad.get_value('joystick_right_x'))
    except NotImplementedError:
        pass

    print(studentapi.get_all_params("studentcode"))

if __name__ == '__main__':
    test_api()
