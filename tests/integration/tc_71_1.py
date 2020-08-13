from tester import *

# setup
start_test("sanity")
start_shm()
start_net_handler()

# poke the system
send_run_mode(SHEPHERD, AUTO)
print_shm()
send_start_pos(SHEPHERD, RIGHT)
print_shm()

#  stop the system
stop_net_handler()
stop_shm()
end_test()
