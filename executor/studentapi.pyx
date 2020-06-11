# cython: nonecheck=True

from cython cimport view, final
from cpython.mem cimport PyMem_Malloc, PyMem_Free
# from pthread cimport *

import enum
import threading
# import numpy as np

"""Student API written in Cython. """

# Initializing logger to be used throughout API
logger_init(STUDENTAPI)
log_runtime(DEBUG, "Student API intialized")


class CodeExecutionError(Exception):
    """
    An exception that occurred while attempting to execute student code

    `CodeExecutionError` accepts arbitrary data that can be examined or written into logs.

    Example:
        >>> err = CodeExecutionError('Error', dev_ids=[2, 3])
        >>> err
        CodeExecutionError('Error', dev_ids=[2, 3])

    """

    def __init__(self, message='', **context):
        super().__init__(message)
        self.context = context
        log_runtime(ERROR, str(self).encode('utf-8'))

    def __repr__(self):
        cls_name, (msg, *_) = self.__class__.__name__, self.args
        kwargs = ', '.join(f'{name}={repr(value)}' for name, value in self.context.items())
        return f'{cls_name}({repr(msg)} {kwargs})'


def _shm_init():
    """ONLY USED FOR TESTING CODE LOADER. NOT USED IN PRODUCTION"""
    shm_init(STUDENTAPI)
    shm_aux_init(STUDENTAPI)

def _shm_stop():
    """ONLY USED FOR TESTING CODE LOADER. NOT USED IN PRODUCTION"""
    shm_stop(STUDENTAPI)
    shm_aux_stop(STUDENTAPI)


def _print(*values, sep=' '):
    """Helper print function that redirects message for stdout instead into the Runtime logger. Will print at the INFO level.

    Args:
        values: iterable of values to print
        sep: string used as the seperator between each value. Default is ' '
    """
    string = sep.join(map(str, values))
    log_runtime(INFO, string.encode('utf-8'))


cdef class Gamepad:
    """
    The API for accessing gamepads.

    Attributes:
        mode: The execution state of the robot.
    """
    cdef public str mode

    def __cinit__(self, mode):
        """Initializes the mode of the robot. Also initializes the auxiliary SHM. """
        self.mode = mode
        shm_aux_init(STUDENTAPI)
        log_runtime(DEBUG, "Aux SHM initialized")

    def __dealloc__(self):
        """Once process is finished and object is deallocated, close the mapping to the auxiliary SHM."""
        shm_aux_stop(STUDENTAPI)
        log_runtime(DEBUG, "Aux SHM stopped")

    cpdef get_value(self, str param_name):
        """
        Get a gamepad parameter if the robot is in teleop.

        Args:
            param: The name of the parameter to read. Possible values are at https://pioneers.berkeley.edu/software/robot_api.html 
        """
        if self.mode != 'teleop':
            raise CodeExecutionError(f'Cannot use Gamepad during {self.mode}')
        cdef bytes param = param_name.encode('utf-8')
        cdef uint32_t buttons
        cdef float joysticks[4]
        cdef char** button_names 
        cdef char** joystick_names 
        with nogil:
            gamepad_read(&buttons, joysticks)
            button_names = get_button_names()
            joystick_names = get_joystick_names()
        for i in range(NUM_GAMEPAD_BUTTONS):
            if param == button_names[i]:
                return bool(buttons & (1 << i))
        for i in range(4):
            if param == joystick_names[i]:
                return joysticks[i]


cdef class Robot:
    """
    The API for accessing the robot and its devices.
    """
    # Include device handler and executor API
    cdef dict running_actions

    def __cinit__(self):
        """Initializes the shared memory (SHM) wrapper. """
        shm_init(STUDENTAPI) # Remove once integrated into executor?
        log_runtime(DEBUG, "SHM intialized")
        self.running_actions = {}

    def __dealloc__(self):
        """Once process is done and object is deallocated, close the mapping to SHM."""
        shm_stop(STUDENTAPI)
        log_runtime(DEBUG, "SHM stopped")
        print("Robot deallocated")


    def run(self, action, *args, **kwargs) -> None:
        """ Schedule an action for execution in a separate thread. Uses Python threading module.
        
        Args:
            action: Python function to run
            args: arguments for the Python function
            kwargs: keyword arguments for the Python function
        """
        thread = threading.Thread(target=action, args=args, kwargs=kwargs)
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
        param_bytes = param_name.encode('utf-8')
        cdef char* param = param_bytes
        splits = device_id.split('_')
        cdef int device_type = int(splits[0])
        cdef uint64_t device_uid = int(splits[1])
        cdef param_val_t* param_value = <param_val_t*> PyMem_Malloc(sizeof(param_val_t) * MAX_PARAMS)
        if not param_value:
            raise MemoryError("Could not allocate memory to get device value.")
        cdef param_desc_t* param_desc
        cdef uint8_t param_idx
        with nogil:
            param_desc = get_param_desc(device_type, param)
            param_idx = get_param_idx(device_type, param)
            device_read_uid(device_uid, EXECUTOR, DATA, 1 << param_idx, param_value)
        param_type = param_desc.type.decode('utf-8')
        if param_type == 'int':
            ret = param_value[param_idx].p_i
        elif param_type == 'float':
            ret = param_value[param_idx].p_f
        elif param_type == 'bool':
            ret = bool(param_value[param_idx].p_b)
        PyMem_Free(param_value)
        return ret


    cpdef void set_value(self, str device_id, str param_name, value):
        """ 
        Set a device parameter.
        
        Args:
            device_id: string of the format '{device_type}_{device_uid}' where device_type is LowCar device ID and      device_uid is 64-bit UID assigned by LowCar.
            param_name: Name of param to get. List of possible values are at https://pioneers.berkeley.edu/software/robot_api.html
            value: Value to set for the param. The type of the value can be seen at https://pioneers.berkeley.edu/software/robot_api.html
        """
        param_bytes = param_name.encode('utf-8')
        cdef char* param = param_bytes
        splits = device_id.split('_')
        cdef int device_type = int(splits[0])
        cdef uint64_t device_uid = int(splits[1])
        cdef param_desc_t* param_desc
        cdef uint8_t param_idx
        with nogil:
            param_desc = get_param_desc(device_type, param)
            param_idx = get_param_idx(device_type, param)
        # Question: Use numpy arrays or cython arrays?
        # cdef param_val_t[:] param_value = np.empty(MAX_PARAMS, dtype=np.dtype([('p_i', 'i4'), ('p_f', 'f4'), ('p_b', 'u1'), ('pad', 'V3')]))
        cdef param_val_t[:] param_value = view.array(shape=(MAX_PARAMS,), itemsize=sizeof(param_val_t), format='ifB')
        param_type = param_desc.type.decode('utf-8')
        if param_type == 'int':
            param_value[param_idx].p_i = value
        elif param_type == 'float':
            param_value[param_idx].p_f = value
        elif param_type == 'bool':
            param_value[param_idx].p_b = int(value)
        with nogil:
            device_write_uid(device_uid, EXECUTOR, COMMAND, 1 << param_idx, &param_value[0])

