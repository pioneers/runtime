import studentapi

# Add tests for API
POLARBEAR = '12_2604648'

def test_api():
    robot = studentapi.Robot()
    val = robot.get_value(POLARBEAR, 'enc_pos')
    print(f"Read value: {val}")

    robot.set_value(POLARBEAR, 'pid_vel_setpoint', 19.023)
    val = robot.get_value(POLARBEAR, 'pid_vel_setpoint')
    print(f"Old value: {val}")
    robot.set_value(POLARBEAR, 'pid_vel_setpoint', 10.0)
    val = robot.get_value(POLARBEAR, 'pid_vel_setpoint')
    print(f"New value: {val}")

if __name__ == '__main__':
    test_api()
