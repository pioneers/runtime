import time
import math

# This autonomous code prints some output every two seconds

def constant_print(msg):
    while True:
        print(f"{msg} printing again")
        time.sleep(2)

def autonomous():
    pass

# This teleop code generates a DivisionByZero Python error

def teleop():
    while True:
        oops = 1 / 0
