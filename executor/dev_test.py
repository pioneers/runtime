import time

Dummy1 = '14_81985529216486895'
Dummy2 = '14_1147797409030816545'

def constant_print():
	while True:
		time.sleep(0.25)
		print('Pief 1: ',  Robot.get_value(Dummy1, 'PIEF'))
		print('Funtime 1: ', Robot.get_value(Dummy1, 'FUNTIME'))
		print('Dawn 2: ', Robot.get_value(Dummy2, 'DAWN'))
		print('Devops 2: ', Robot.get_value(Dummy2, 'DEVOPS'))

def constant_write():
	val = 12
	fval = 0.0
	while True:
		time.sleep(1)
		Robot.set_value(Dummy1, 'PIEF', val)
		Robot.set_value(Dummy1, 'FUNTIME', fval)
		val += 2
		fval += 3.14

def autonomous_setup():
	Robot.run(constant_print)
	Robot.run(constant_write)

def autonomous_main():
	pass

def teleop_setup():
	pass

def teleop_main():
	pass
