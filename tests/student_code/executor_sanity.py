import time
import math

# This autonomous code prints some output every two seconds

def constant_print(msg):
	while True:
		print(f"{msg} printing again")
		time.sleep(2)

def autonomous():
	print('Autonomous setup has begun!')
	print(f"Starting position: {Robot.start_pos}")
	Robot.run(constant_print, "autonomous")

# This teleop code generates a DivisionByZero Python error

def teleop():
	oops = 1 / 0
