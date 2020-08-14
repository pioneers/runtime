from tester import *

# setup
start_test("nonexistent device subscription")
start_shm()
start_net_handler()

# poke
data = [
    [50, "ServoControl", 0b11],
    [100, "LimitSwitch", 0b101]
]
send_device_data(data)

# stop
stop_net_handler()
stop_shm()
end_test()

output1 = "no device at dev_uid = 50, sub request failed"
output2 = "Invalid device subscription: device uid 50 is invalid"
output3 = "no device at dev_uid = 100, sub request failed"
output4 = "Invalid device subscription: device uid 100 is invalid"

# check output
assert_output(output1, True)
assert_output(output2, True)
assert_output(output3, True)
assert_output(output4, True)
