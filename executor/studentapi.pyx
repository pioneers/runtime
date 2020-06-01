# cython: nonecheck=True

from cython cimport view
from cpython.mem cimport PyMem_Malloc, PyMem_Free
# from pthread cimport *

import enum
# import numpy as np

"""Student API written in Cython. To compile to C, do `python3.6 setup.py build_ext -i` in this directory. """

logger_init(API)
log_runtime(DEBUG, "Student API intialized")


class CodeExecutionError(Exception):
    """
    An exception that occurred while attempting to execute student code

    ``CodeExecutionError`` accepts arbitrary data that can be examined in
    post-mortems or written into structured logs.

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


def _print(*values, sep=' '):
    string = sep.join(map(str, values))
    log_runtime(INFO, string.encode('utf-8'))


cdef class Gamepad:
    """
    The API for accessing gamepads.

    Attributes:
        mode: The execution state of the robot.
    """
    cdef str mode

    def __cinit__(self, mode):
        self.mode = mode

    def __dealloc__(self):
        pass

    cpdef get_value(self, str param_name):
        """
        Get a gamepad parameter if the robot is in teleop.

        Attributes:
            param: The name of the parameter to read.
            gamepad_id: The gamepad index (defaults to the first gamepad).
        """
        if self.mode != 'teleop':
            raise CodeExecutionError(f'Cannot use Gamepad during {self.mode}')
        cdef bytes param = param_name.encode('utf-8')

cdef class Robot:
    """
    The API for accessing the robot and its sensors.

    Attributes:
        action_executor: An action executor.
        aliases: A device alias manager.
    """
    # Include device handler and executor API

    def __cinit__(self):
        shm_init(EXECUTOR) # Remove once integrated into 
        log_runtime(DEBUG, "SHM intialized")

    def __dealloc__(self):
        shm_stop(EXECUTOR)
        log_runtime(DEBUG, "SHM stopped")


    cpdef void run(self, action, args, int timeout=300):
        """ Schedule an action for execution in a separate thread. """
        pass


    cpdef bint is_running(self, action):
        pass


    cpdef get_value(self, str device_id, str param_name):
        """ Get a smart sensor parameter. """
        cdef bytes param = param_name.encode('utf-8')
        splits = device_id.split('_')
        device_type, device_uid = int(splits[0]), int(splits[1])
        idx = get_device_idx_from_uid(device_uid)
        cdef param_desc_t* param_desc = get_param_desc(device_type, param)
        param_idx = get_param_idx(device_type, param)
        # print(param_desc.type)
        # print(param_idx)
        cdef param_val_t* param_value = <param_val_t*> PyMem_Malloc(sizeof(param_val_t) * MAX_PARAMS)
        if not param_value:
            raise MemoryError()
        cdef uint32_t param_mask = UINT32_MAX
        device_read(idx, EXECUTOR, DATA, param_mask, param_value)
        # print_params(param_mask, param_value)
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
        """ Set a smart sensor parameter. """
        cdef bytes param = param_name.encode('utf-8')
        splits = device_id.split('_')
        device_type, device_uid = int(splits[0]), int(splits[1])
        idx = get_device_idx_from_uid(device_uid) 
        cdef param_desc_t* param_desc = get_param_desc(device_type, param)
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
        # print(param_desc.type)
        # print(param_idx)
        cdef uint32_t param_mask = 1 << param_idx
        device_write(idx, EXECUTOR, COMMAND, param_mask, &param_value[0])

