
### External declarations from test clients, to be used by test cases

from libc.stdint cimport *
from cpython.mem cimport PyMem_Malloc, PyMem_Free

cdef extern from "../runtime_util/runtime_util.h":
    cpdef int MAX_DEVICES
    cpdef int MAX_PARAMS
    cpdef int NUM_GAMEPAD_BUTTONS
    char** get_button_names()
    char** get_joystick_names()

cpdef enum process_t:
    EXECUTOR, NET_HANDLER, DEV_HANDLER, SHM, TEST
cpdef enum param_type_t:
    INT, FLOAT, BOOL
cpdef enum robot_desc_field_t:
    RUN_MODE, DAWN, SHEPHERD, GAMEPAD, START_POS
cpdef enum robot_desc_val_t:
    IDLE, AUTO, TELEOP, CHALLENGE, CONNECTED, DISCONNECTED, LEFT, RIGHT

cdef extern from "client/executor_client.h":
    void c_start_executor "start_executor" (char* student_code)
    cpdef void stop_executor()

cdef extern from "client/shm_client.h":
    cpdef void start_shm()
    cpdef void stop_shm()
    cpdef void print_shm()

cdef extern from "client/net_handler_client.h":
    ctypedef struct dev_data_t:
        uint64_t uid
        char* name
        uint32_t params
    cpdef void start_net_handler()
    cpdef void stop_net_handler()
    cpdef void send_run_mode(robot_desc_field_t, robot_desc_val_t)
    cpdef void send_start_pos(robot_desc_field_t, robot_desc_val_t)
    void c_send_gamepad_state "send_gamepad_state" (uint32_t buttons, float joystick_vals[4])
    void c_send_challenge_data "send_challenge_data" (robot_desc_field_t client, char **data)
    void c_send_device_data "send_device_data" (dev_data_t *data, int num_devices)
    cpdef void print_next_dev_data ()


cpdef start_executor(str student_code):
    c_start_executor(student_code.encode('utf-8'))

cpdef send_gamepad_state(int buttons, list joystick):
    cdef float joy_array[4]
    for i in range(len(joy_array)):
        joy_array[i] = joystick[i]
    c_send_gamepad_state(buttons, joy_array)

cpdef send_challenge_data(robot_desc_field_t client, list data):
    cdef char** inputs = <char**> PyMem_Malloc(sizeof(char*) * len(data))
    cdef bytes temp
    for i in range(len(data)):
        temp = data[i].encode('utf-8')
        inputs[i] = temp
    c_send_challenge_data(client, inputs)
    PyMem_Free(inputs)

cpdef send_device_data(list data):
    cdef dev_data_t* device_data = <dev_data_t*> PyMem_Malloc(sizeof(dev_data_t) * len(data))
    for i in range(len(data)):
        device_data[i].uid = data[i][0]
        device_data[i].name = data[i][1]
        device_data[i].params = data[i][2]
    c_send_device_data(device_data, len(data))
    PyMem_Free(device_data)

### Test Assertion

import sys
import io
import os

terminal_fd = os.dup(sys.stdout.fileno())
test_output = None # Populated by end_test()
TEMP_FILE = '/tmp/temp_logs.txt'

def start_test(desc: str):
    print("Starting test:", desc)
    with open(TEMP_FILE, 'w') as f:
        os.dup2(f.fileno(), sys.stdout.fileno())

def end_test():
    with open(TEMP_FILE, 'r') as f:
        test_output = f.readlines()
    os.dup2(terminal_fd, sys.stdout.fileno())
    print(f"All test output:\n{test_output}")

def in_output(expected: str):
    expected = expected.strip()
    for line in test_output:
        line = line.strip()
        if expected in line:
            return True
    return False

