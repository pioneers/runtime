from tester import *
from time import sleep

# set everything up
start_test("executor sanity test")
start_shm()
start_net_handler()
start_executor("executor_sanity")

# poke the system
# this section checks the autonomous code (should generate some print statements)
send_start_pos(SHEPHERD, RIGHT)
send_run_mode(SHEPHERD, AUTO)
sleep(1)
print_shm()
sleep(2)
send_run_mode(SHEPHERD, IDLE)
print_shm()

# this section checks the teleop code (should generate division by zero error)
send_run_mode(DAWN, TELEOP)
print_shm()
send_run_mode(DAWN, IDLE)
print_shm()

# this section runs the coding challenges (should not error or time out)
inputs = ["2039", "190172344"]
send_challenge_data(DAWN, inputs)

# stop all the processes
stop_executor()
stop_net_handler()
stop_shm()
end_test()


output1 = "Autonomous setup has begun!"
output2 = "autonomous printing again"
output3 = "RUN_MODE = AUTO"
output4 = "RUN_MODE = IDLE"
output5 = "Traceback (most recent call last):"
# we have to skip the File: <file path> because on the pi it's /home/pi/c-runtime but on Travis it's /root/runtime
output6 = """line 25, in teleop_main
    oops = 1 / 0
    ZeroDivisionError: division by zero"""
output7 = "Python function teleop_main call failed"
output8 = "RUN_MODE = TELEOP"
output9 = """Challenge 0 result: 9302
	         Challenge 1 result: [2, 661, 35963]"""
output10 = "Suppressing output: too many messages..."

# check outputs
assert_output(output1, True)
assert_output(output2, True)
assert_output(output3, True)
assert_output(output4, True)
assert_output(output5, True)
assert_output(output6, True)
assert_output(output7, True)
assert_output(output8, True)
assert_output(output9, True)
assert_output(output10, False, exclude=True) # check to make sure we don't get the suppressing messages bug
