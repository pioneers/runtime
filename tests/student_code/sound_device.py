# Student code that plays pitches from keyboard inputs
# Commented out print_buttons and play_notes functions as they are not run 
import time

SOUND = '59_1'

################################# GLOBAL VARS ##################################

NOTES = 'CC GG AA G  FF EE DD C'
PAUSE_DURATION = 0.125  # The duration (s) to wait during a whitespace
NOTE_DURATION = 0.75    # The duration that a note is held before playing the next

# Mapping notes to pitches
# https://www.szynalski.com/tone-generator/
MAP = {
    'C' : 261,
    'D' : 294,
    'E' : 330,
    'F' : 349,
    'G' : 392,
    'A' : 440,
    'B' : 494
}

################################## AUTONOMOUS ##################################

def autonomous():
    print("Now executing AUTONOMOUS")
    # Write pitches
    for note in NOTES:
        if (note == ' '):
            print("Pause")
            time.sleep(PAUSE_DURATION)
        else:
            print("Writing", note)
            Robot.set_value(SOUND, "PITCH", MAP[note])
            time.sleep(NOTE_DURATION)

#################################### TELEOP ####################################

def teleop():
    print("Now executing TELEOP")
    # Robot.run(print_button)
    # Robot.run(play_notes)
    while True:
        if Gamepad.get_value('button_a'):
            Robot.set_value(SOUND, "PITCH", MAP['C'])
            print("Wrote Button A: Pitch C")
            time.sleep(NOTE_DURATION);
        if Gamepad.get_value('button_b'):
            Robot.set_value(SOUND, "PITCH", MAP['B'])
            print("Wrote Button B: Pitch B")
            time.sleep(NOTE_DURATION);

################################### THREADS ####################################

# def print_button():
#    while (1):
#        if Gamepad.get_value('button_a'):
#            print("BUTTON A IS PRESSED")
#        if Gamepad.get_value('button_b'):
#            print("BUTTON B IS PRESSED")

# def play_notes():
#     while (1):
#         if Gamepad.get_value('button_a'):
#             Robot.set_value(SOUND, "PITCH", MAP['A'])
#         if Gamepad.get_value('button_b'):
#             Robot.set_value(SOUND, "PITCH", MAP['B'])
