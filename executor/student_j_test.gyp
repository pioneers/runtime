import studentapi

SOUND = "59_1"
NOTE_DURATION = "1000"

def autonomous_setup():
    print ("Autonomous Setup")
    Robot.set_value(SOUND, "PITCH", MAP['C'])
    print("Pitch C")
    time.sleep(NOTE_DURATION)
    Robot.set_value(SOUND, "PITCH", MAP['B'])

def autonomous_main():
    pass

def teleop_setup():
    print ("Teleop Setup")
    pass

def teleop_main():
    if Gamepad.get_value('button_a'):
        Robot.set_value(SOUND, "PITCH", MAP['C'])
        print("Wrote Button A: Pitch C")
        time.sleep(NOTE_DURATION)
    if Gamepad.get_value('button_b'):
        Robot.set_value(SOUND, "PITCH", MAP['B'])
        print("Wrote Button B: Pitch B")
        time.sleep(NOTE_DURATION)
