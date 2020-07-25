import time

def constant_print(msg):
	while True:
		print(f"{msg} printing again")
		time.sleep(2)

def autonomous_setup():
	print('Autonomous setup has begun!')
	print(f"Starting position: {Robot.start_pos}")
	Robot.run(constant_print, "autonomous")

def autonomous_main():
	pass

def teleop_setup():
	pass

def teleop_main():
	pass
