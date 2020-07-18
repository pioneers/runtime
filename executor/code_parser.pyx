from runtime cimport get_device, MAX_DEVICES, MAX_PARAMS, place_sub_request, EXECUTOR, log_printf, DEBUG, device_t, uint32_t

import importlib
import sys
from collections import defaultdict

""" Student code parser to determine params used """

cpdef void make_device_subs(str code_file):
    cdef device_t* device
    cdef uint32_t request
    cdef int err
    params = get_all_params(code_file)
    for id in params:
        splits = id.split('_')
        type, uid = int(splits[0]), int(splits[1])
        device = get_device(type)
        if not device:
            log_printf(DEBUG, f"Device with uid {uid} has invalid type {type}".encode('utf-8'))
            continue
        request = 0
        for p in params[id]:
            for i in range(device.num_params):
                if device.params[i].name == p.encode('utf-8'):
                    log_printf(DEBUG, f"adding request for device {id} param {p} at index {i}".encode('utf-8'))
                    request |= (1 << i)
        err = place_sub_request(uid, EXECUTOR, request)
        if err == -1:
            log_printf(DEBUG, f"Device with type {type} and uid {uid} is not connected to the robot".encode('utf-8'))

def get_all_params(code_file):
    code = importlib.import_module(code_file)
    var = [n for n in dir(code) if not n.startswith("_")]
    mod = sys.modules[__name__]
    for v in var:
        setattr(mod, v, getattr(code, v))
    with open(f"{code_file}.py", "r") as f:
        for line in f:
            if "Robot.set_value" in line or "Robot.get_value" in line:
                line = line.lstrip()
                comment = line.find("#")
                if comment != -1:
                    line = line[:comment]
                line = line.replace('Robot', 'MockRobot')
                line = line.replace('Gamepad', 'MockGamepad')
                exec(line)
    return MockRobot.device_params

class MockRobot:
    device_params = defaultdict(set)

    @classmethod
    def get_value(cls, id, param):
        cls.device_params[id].add(param)
        return 0

    @classmethod
    def set_value(cls, id, param, val):
        cls.device_params[id].add(param)

class MockGamepad:
    @classmethod
    def get_value(self, param):
        return 0
