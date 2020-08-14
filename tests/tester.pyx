
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
cpdef enum gp_gamepad:
    BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y, L_BUMPER, R_BUMPER, L_TRIGGER, R_TRIGGER, \
	BUTTON_BACK, BUTTON_START, L_STICK, R_STICK, DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT, BUTTON_XBOX
cpdef enum gp_joystick:
    JOYSTICK_LEFT_X, JOYSTICK_LEFT_Y, JOYSTICK_RIGHT_X, JOYSTICK_RIGHT_Y

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
    cdef bytes temp
    for i in range(len(data)):
        device_data[i].uid = data[i][0]
        temp = data[i][1].encode('utf-8')
        device_data[i].name = temp
        device_data[i].params = data[i][2]
    c_send_device_data(device_data, len(data))
    PyMem_Free(device_data)

### Test Assertion

import sys
import io
import os
import re
import threading
import time
from typing import List, Optional

terminal_fd = os.dup(sys.stdout.fileno())
TEMP_FILE = '/tmp/temp_logs.txt'

test_output = None # Populated by end_test()
current_idx = 0
check_num = 0
stop_print = threading.Event()

def output_print():
    with open(TEMP_FILE, 'r') as f:
        while not stop_print.is_set():
            # print(f.readline(), end='', file=sys.stderr)
            pass

thread = threading.Thread(target=output_print)


def start_test(desc: str):
    print(f"################ Starting test: {desc} ################")
    with open(TEMP_FILE, 'w') as f:
        os.dup2(f.fileno(), sys.stdout.fileno()) # Redirect stdout to the temp file we opened
    thread.start()


def end_test():
    stop_print.set()
    thread.join()
    with open(TEMP_FILE, 'r') as f:
        global test_output
        test_output = f.readlines()
    os.remove(TEMP_FILE)
    os.dup2(terminal_fd, sys.stdout.fileno()) # Revert stdout back to the terminal screen
    print()
    print(f"\n{''.join(test_output)}")


def _clean_string(string: str):
    string = string.strip()
    string = re.sub(r"\s+", '', string)
    return string


def assert_output(expected: str, remaining: bool = False, exclude: bool = False):
    global check_num
    check_num += 1

    expected_lines: List[str] = expected.split('\n')
    start: str = _clean_string(expected_lines[0])
    failed_line: Optional[str] = None
    last_found: Optional[str] = None

    for i in range(current_idx if remaining else 0, len(test_output)):
        line = _clean_string(test_output[i])
        if ((start not in line) if exclude else (start in line)): # Find the first location where `expected` does/doesn't occur
            passed = True
            for j in range(len(expected_lines)):
                line = _clean_string(expected_lines[j])
                output = _clean_string(test_output[i + j])
                if ((line in output) if exclude else (line not in output)):
                    passed = False
                    failed_line = line
                    last_found = output
                    # print(f"Expected: {line}\nActual: {output}")
                    break
            if remaining:
                global current_idx
                current_idx = i + len(expected_lines)
            if passed:
                print(f"Check {check_num} passed")
                return

    print(f"Check {check_num} failed\n")
    print(f"Last failed while{' not' if exclude else ''} expecting:\n{failed_line or start}\n")
    if last_found:
        print(f"Instead found:\n{last_found}")
    print()
    sys.exit(1)

