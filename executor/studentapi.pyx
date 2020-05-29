# cython: nonecheck=True

from cpython.mem cimport PyMem_Malloc, PyMem_Free
# from pthread cimport *

import enum
import typing
import functools
import importlib

"""Student API written in Cython. To compile to C, do `python3.6 setup.py build_ext -i` in this directory. """

logger_init(API)
log_runtime(DEBUG, "Student API intialized")

class RuntimeBaseException(Exception):
    """
    Base for Runtime-specific exceptions.

    ``RuntimeBaseException`` accepts arbitrary data that can be examined in
    post-mortems or written into structured logs.

    Example:

        >>> err = RuntimeBaseException('Error', dev_ids=[2, 3])
        >>> err
        RuntimeBaseException('Error', dev_ids=[2, 3])

    """

    def __init__(self, message, **context):
        super().__init__(message)
        self.context = context

    def __repr__(self):
        cls_name, (msg, *_) = self.__class__.__name__, self.args
        if self.context:
            kwargs = ', '.join(f'{name}={repr(value)}' for name, value in self.context.items())
            return f'{cls_name}({repr(msg)}, {kwargs})'
        return f'{cls_name}({repr(msg)})'


class RuntimeExecutionError(RuntimeBaseException):
    """ An exception that occurred while attempting to execute student code. """


cdef class Gamepad:
    """
    The API for accessing gamepads.

    Attributes:
        mode: The execution state of the robot.
    """
    mode = IDLE

    cpdef get_value(self, param: str, gamepad_id: int = 0):
        """
        Get a gamepad parameter if the robot is in teleop.

        Attributes:
            param: The name of the parameter to read.
            gamepad_id: The gamepad index (defaults to the first gamepad).
        """
        if self.mode is not TELEOP:
            raise RuntimeExecutionError(f'Cannot use Gamepad during {self.mode}',
                                        mode=self.mode, gamepad_id=gamepad_id,
                                        param=param)
        return super()._get_value(f'gamepad-{gamepad_id}', param)


cdef class Robot:
    """
    The API for accessing the robot and its sensors.

    Attributes:
        action_executor: An action executor.
        aliases: A device alias manager.
    """
    # Include device handler and executor API

    cpdef run(self, action, args, int timeout=300):
        """ Schedule an action (coroutine function) for execution in a separate thread. """
        # self.action_executor.register_action_threadsafe(action, *args, timeout=timeout)
        pass


    # def _normalize_device_uid(self, device_uid: typing.Union[str, int]) -> str:
    #     """ Translate device aliases into UIDs and return a device mapping key. """
    #     device_uid = str(device_uid)
    #     if device_uid in self.aliases:
    #         device_uid = self.aliases[device_uid]
    #     return f'smart-sensor-{device_uid}'

    cpdef get_value(self, str device_id, str param_name):
        """ Get a smart sensor parameter. """
        cdef bytes param = param_name.encode('utf-8')
        device_type, device_uid = int(str(device_id)[:20]), int(str(device_id)[20:])
        log_runtime(DEBUG, f"device_type {device_type} device_uid {device_uid}".encode('utf-8'))
        idx = get_device_idx_from_uid(device_uid)
        log_runtime(DEBUG, f"got {idx}".encode('utf-8'))
        cdef param_desc_t* param_desc = get_param_desc(device_type, param)
        param_id = get_param_id(device_type, param)
        cdef param_val_t* param_value = <param_val_t*> PyMem_Malloc(sizeof(param_val_t)*MAX_PARAMS)
        if not param_value:
            raise MemoryError()
        cdef uint32_t param_mask = 1 << param_id
        device_read(idx, EXECUTOR, DATA, param_mask, param_value)
        param_type = param_desc.type.decode('utf-8')
        if param_type == 'int':
            ret = param_value.p_i
        elif param_type == 'float':
            ret = param_value.p_f
        elif param_type == 'bool':
            ret = bool(param_value.p_b)
        PyMem_Free(param_value)
        return ret

    cpdef set_value(self, device_id, param, value):
        """ Set a smart sensor parameter. """
        pass
        # try:
        #     device.struct.set_desired(param, value)
        # except AttributeError as exc:
        #     raise RuntimeExecutionError(
        #         'Unrecognized or read-only parameter',
        #         device_uid=device_uid,
        #         device_type=type(device.struct).__name__,
        #         param=param,
        #     ) from exc

    def get_alliance(self):
        pass

    def get_mode(self):
        pass
