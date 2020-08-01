import importlib
import sys
import re
from collections import defaultdict

""" Student code parser to determine params used """

cpdef void make_device_subs(str code_file) except *:
    cdef device_t* device
    cdef uint32_t request
    cdef int err
    params = get_all_params(code_file)
    for id in params:
        splits = id.split('_')
        type, uid = int(splits[0]), int(splits[1])
        device = get_device(type)
        if not device:
            log_printf(WARN, f"Code parser: device with uid {uid} has invalid type {type}".encode('utf-8'))
            continue
        request = 0
        for p in params[id]:
            for i in range(device.num_params):
                if device.params[i].name == p.encode('utf-8'):
                    # log_printf(DEBUG, f"adding request for device {id} param {p} at index {i}".encode('utf-8'))
                    request |= (1 << i)
        err = place_sub_request(uid, EXECUTOR, request)
        if err == -1:
            log_printf(WARN, f"Code parser: device with type {type} and uid {uid} is not connected to the robot".encode('utf-8'))

def get_all_params(code_file):
    code = importlib.import_module(code_file)
    # Finds all global variables in student code
    var = [n for n in dir(code) if not n.startswith("_")]
    mod = sys.modules[__name__]
    # Sets them in current global namespace
    for v in var:
        setattr(mod, v, getattr(code, v))
    param_dict = defaultdict(set)

    # Open the code file into f (try the current directory and the test student code directory)
    try:
        f = open(f"{code_file}.py", "r")
    except FileNotFoundError as e:
        f = open(f"../tests/student_code/{code_file}.py", "r")	
	
    for i, line in enumerate(f):
        line = line.lstrip() # Remove whitespace
        comment = line.find("#")
        if comment != -1:
            line = line[:comment] # Remove commented lines
        # Find exact functions
        matches = [re.search(r"Robot.set_value\((.*)\)", line), re.search(r"Robot.get_value\((.*)\)", line)]
        for res in matches:
            if res:
                try:
                    # Find parameters by splitting by comma
                    params = re.split(r"\s?[,\(\)]\s?", res[1])
                    param_dict[eval(params[0])].add(eval(params[1]))
                except Exception as e:
                    log_printf(DEBUG, f"Error parsing student code on line {i}: {str(e)}".encode())
    return param_dict
