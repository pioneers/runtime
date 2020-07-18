# cython: nonecheck=True

# from libc.stdint cimport *
from runtime cimport *
from cpython.mem cimport PyMem_Malloc, PyMem_Free

import threading
import sys
import builtins

include "code_parser.pyx"

"""Student API written in Cython. """

MAX_THREADS = 8


## Tools used for logging

def _print(*values, sep=' ', end='\n', file=None, flush=None, level=INFO):
    """Helper print function that redirects message for stdout instead into the Runtime logger. Will print at the INFO level.

    Args:
        values: iterable of values to print
        sep: string used as the seperator between each value. Default is ' '
    """
    string = sep.join(map(str, values)) + end
    if file == sys.stdout:
        level = INFO
    elif file == sys.stderr:
        level = ERROR
    log_printf(level, string.encode('utf-8'))

class OutputRedirect:
    def __init__(self, level):
        self.level = level
    def write(self, text):
        log_printf(self.level, text.encode('utf-8'))

builtins.print = _print
sys.stderr = OutputRedirect(PYTHON)


## Test functions for SHM

def _init():
    """ONLY USED FOR TESTING. NOT USED IN PRODUCTION"""
    logger_init(EXECUTOR)
    shm_init()

## API Objects

class DeviceError(Exception):
    """An exception caused by using an invalid device. """


cdef class Gamepad:
    """
    The API for accessing gamepads.

    Attributes:
        mode: The execution state of the robot.
    """
    cdef bint available

    def __cinit__(self):
        """Initializes the mode of the robot. """
        self.available = robot_desc_read(RUN_MODE) == TELEOP


    cpdef get_value(self, str param_name):
        """
        Get a gamepad parameter if the robot is in teleop.

        Args:
            param: The name of the parameter to read. Possible values are at https://pioneers.berkeley.edu/software/robot_api.html 
        """
        if not self.available:
            raise NotImplementedError(f'Can only use Gamepad during teleop mode')
        # Convert Python string to C string
        cdef bytes param = param_name.encode('utf-8')
        cdef uint32_t buttons
        cdef float joysticks[4]
        cdef int err = gamepad_read(&buttons, joysticks)
        if err == -1:
            raise DeviceError(f"Gamepad isn't connected to Dawn or the robot")
        cdef char** button_names = get_button_names()
        cdef char** joystick_names = get_joystick_names()
        # Check if param is button
        for i in range(NUM_GAMEPAD_BUTTONS):
            if param == button_names[i]:
                return bool(buttons & (1 << i))
        # Check if param is joystick
        for i in range(4):
            if param == joystick_names[i]:
                return joysticks[i]
        raise DeviceError(f"Invalid gamepad parameter {param_name}")


class ThreadWrapper(threading.Thread):

    def __init__(self, action, args, kwargs):
        super().__init__()
        self.action = action
        self.args = args
        self.kwargs = kwargs

    def run(self):
        _print(f"Action Thread ID: {self.ident}", level=DEBUG)
        try:
            self.action(*self.args, **self.kwargs)
        except TimeoutError:
            pass


cdef class Robot:
    """
    The API for accessing the robot and its devices.
    """
    cdef dict running_actions
    cdef public str start_pos

    def __cinit__(self):
        """Initializes the dict of running threads. """
        self.running_actions = {}
        self.start_pos = 'left' if robot_desc_read(START_POS) == LEFT else 'right'


    def run(self, action, *args, **kwargs) -> None:
        """ Schedule an action for execution in a separate thread. Uses Python threading module.
        
        Args:
            action: Python function to run
            args: arguments for the Python function
            kwargs: keyword arguments for the Python function
        """
        if self.is_running(action):
            _print(f"Calling action {action.__name__} when it is still running won't do anything. Use Robot.is_running to check if action is over.", level=WARN)
            return
        if threading.active_count() > MAX_THREADS:
            _print(f"Number of Python threads {threading.active_count()} exceeds the limit {MAX_THREADS} so action won't be scheduled. Make sure your actions are returning properly.", level=WARN)
            return
        thread = ThreadWrapper(action, args, kwargs)
        thread.daemon = True
        self.running_actions[action.__name__] = thread
        thread.start()
        

    def is_running(self, action) -> bool:
        """Returns whether the given function `action` is running in a different thread.
        
        Args:
            action: Python function to check
        """
        thread = self.running_actions.get(action.__name__, None)
        if thread:
            return thread.is_alive()
        return False


    cpdef get_value(self, str device_id, str param_name):
        """ 
        Get a device value. 
        
        Args:
            device_id: string of the format '{device_type}_{device_uid}' where device_type is LowCar device ID and      device_uid is 64-bit UID assigned by LowCar.
            param_name: Name of param to get. List of possible values are at https://pioneers.berkeley.edu/software/robot_api.html
        """
        # Convert Python string to C string
        cdef bytes param = param_name.encode('utf-8')

        # Getting device identification info
        splits = device_id.split('_')
        cdef int device_type = int(splits[0])
        cdef uint64_t device_uid = int(splits[1])

        # Getting parameter info from the name
        cdef device_t* device = get_device(device_type)
        if not device:
            raise DeviceError(f"Device with uid {device_uid} has invalid type {device_type}")
        cdef str param_type
        cdef int8_t param_idx = -1
        for i in range(device.num_params):
            if device.params[i].name == param:
                param_idx = i
                param_type = device.params[i].type.decode('utf-8')
                break
        if param_idx == -1:
            raise DeviceError(f"Invalid device parameter {param_name} for device type {device.name.decode('utf-8')}({device_type})")

        # Allocate memory for parameter
        cdef param_val_t* param_value = <param_val_t*> PyMem_Malloc(sizeof(param_val_t) * MAX_PARAMS)
        if not param_value:
            raise MemoryError("Could not allocate memory to get device value.")

        # Read and return parameter
        cdef int err = device_read_uid(device_uid, EXECUTOR, DATA, 1 << param_idx, param_value)
        if err == -1:
            PyMem_Free(param_value)
            raise DeviceError(f"Device with type {device.name.decode('utf-8')}({device_type}) and uid {device_uid} isn't connected to the robot")

        if param_type == 'int':
            ret = param_value[param_idx].p_i
        elif param_type == 'float':
            ret = param_value[param_idx].p_f
        elif param_type == 'bool':
            ret = bool(param_value[param_idx].p_b)
        PyMem_Free(param_value)
        return ret


    cpdef void set_value(self, str device_id, str param_name, value) except *:
        """ 
        Set a device parameter.
        
        Args:
            device_id: string of the format '{device_type}_{device_uid}' where device_type is LowCar device ID and      device_uid is 64-bit UID assigned by LowCar.
            param_name: Name of param to get. List of possible values are at https://pioneers.berkeley.edu/software/robot_api.html
            value: Value to set for the param. The type of the value can be seen at https://pioneers.berkeley.edu/software/robot_api.html
        """
        # Convert Python string to C string
        cdef bytes param = param_name.encode('utf-8')

        # Get device identification info
        splits = device_id.split('_')
        cdef int device_type = int(splits[0])
        cdef uint64_t device_uid = int(splits[1])

        # Getting parameter info from the name
        cdef device_t* device = get_device(device_type)
        if not device:
            raise DeviceError(f"Device with uid {device_uid} has invalid type {device_type}")
        cdef str param_type
        cdef int8_t param_idx = -1
        for i in range(device.num_params):
            if device.params[i].name == param:
                param_idx = i
                param_type = device.params[i].type.decode('utf-8')
                break
        if param_idx == -1:
            raise DeviceError(f"Invalid device parameter {param_name} for device type {device.name.decode('utf-8')}({device_type})")

        # Allocating memory for parameter to write
        cdef param_val_t* param_value = <param_val_t*> PyMem_Malloc(sizeof(param_val_t) * MAX_PARAMS)
        if not param_value:
            raise MemoryError("Could not allocate memory to get device value.")

        if param_type == 'int':
            param_value[param_idx].p_i = value
        elif param_type == 'float':
            param_value[param_idx].p_f = value
        elif param_type == 'bool':
            param_value[param_idx].p_b = int(value)
        cdef int err = device_write_uid(device_uid, EXECUTOR, COMMAND, 1 << param_idx, &param_value[0])
        PyMem_Free(param_value)
        if err == -1:
            raise DeviceError(f"Device with type {device.name.decode('utf-8')}({device_type}) and uid {device_uid} isn't connected to the robot")

