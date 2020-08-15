import time
import math

MOTOR = '13_2652141'

def autonomous_actions(n=1000):
    print('Running autonomous action ...')
    for i in range(n):
        print(f'Doing action computation ({i+1}/{n}) ...')
        Robot.sleep(0.5)


def set_motor():
    Robot.set_value(MOTOR, 'duty_cycle_a', 0.5)
    Robot.sleep(1)
    print("after first sleep")
    Robot.set_value(MOTOR, 'duty_cycle_b', 0)
    Robot.sleep(2)
    print("after second sleep")
    while True:
        Robot.set_value(MOTOR, 'pid_kp_a', 0.237)

def constant_print(msg):
    while True:
        print(f"{msg} printing again")
        Robot.sleep(2)


def autonomous_setup():
    print('Autonomous setup has begun!')
    print(f"Starting position: {Robot.start_pos}")
    
    # Robot.run(autonomous_actions)
    # Robot.run(set_motor)
    Robot.set_value(MOTOR, 'duty_cycle_a', 0.2)
    global start
    start = time.time()
    Robot.run(set_motor)
    Robot.run(constant_print, "auton")
    global i
    i = 0
    Robot.sleep(10)

def autonomous_main():
    # Robot.sleep(10)
    # Robot.run(wait)
    Robot.get_value(MOTOR, 'duty_cycle_b')
    global i, start
    if i % 10000 == 0:
        print("Iteration:", i, time.time() - start)
        Robot.log("iteration time", time.time() - start)
        start = time.time()
        # Robot.run(double, 5.0)
    i += 1
    Robot.set_value(MOTOR, 'enc_b', .12)
    
    # while True: pass
    # print("testing whether thread dies", time.time())
    # print('Running autonomous main ...')
    # start = time.time()
    # print('I wrote an infinite loop')
    # while True:
    #     print(f'Teleop main has been running for {round(time.time() - start, 3)}s')
    #     Robot.sleep(0.1)
    #
    # x()
    #
    # Robot.run(autonomous_actions)


def teleop_setup():
    print('Teleop setup has begun!')
    global start
    start = time.time()
    global i
    i = 0


def teleop_main():
    # print('Running teleop main ...')
    # print(Gamepad.get_value('a'))
    # print('A -> ', Gamepad.get_value('button_a'))
    # print('B -> ', Gamepad.get_value('button_b'))
    # print('X -> ', Gamepad.get_value('button_x'))
    # print('Y -> ', Gamepad.get_value('button_y'))
    # print('Xbox -> ', Gamepad.get_value('button_xbox'))
    # print('Dpad up -> ', Gamepad.get_value('dpad_up'))
    # print('joystick_left_x -> ', Gamepad.get_value('joystick_left_x'))
    # print('joystick_right_y -> ', Gamepad.get_value('joystick_right_y'))

    # print('=>', Robot.get_value('my_rfid', 'id'), Robot.get_value('my_rfid2', 'tag_detect'))
    
    Robot.set_value(MOTOR, 'duty_cycle_b', Gamepad.get_value('joystick_left_x'))
    pos = Robot.get_value(MOTOR, 'enc_a')

    if Gamepad.get_value('button_a'):
        global i, start 
        if i % 10000 == 0:
            print("Iteration:", i, time.time() - start)
            start = time.time()
            # Robot.run(double, 100)
        i += 1
        # print('in here')
    # print('Running motor ...')
    Robot.set_value(MOTOR, 'duty_cycle_a', .12)

# Times out
# def autonomous_main():
#     print('Running autonomous main ...')
#     autonomous_main()


def double(x):
    return 2 * x


def wait():
    while True:
        Robot.get_value(MOTOR, 'enc_b')
        # print('of course')
