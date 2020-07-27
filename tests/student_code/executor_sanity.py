import time
import math

# This autonomous code prints some output every two seconds

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

# This teleop code generates a DivisionByZero Python error

def teleop_setup():
	pass

def teleop_main():
	oops = 1 / 0

# Two coding challenges for testing

def reverse_digits(num):
    return int(str(num)[::-1])


def list_prime_factors(num):
    primes = []
    for i in range(2, int(math.sqrt(num))):
        while num % i == 0:
            add = True
            for p in primes:
                if i % p == 0:
                    add = False
                    break
            if add:
                primes.append(i)
            num = num // i
    if num not in primes:
        primes.append(num)
    return primes
