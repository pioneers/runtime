# cython: nonecheck=True
import cython

# from runtime_api cimport *

import enum
import typing
import functools

"""Student API written in Cython. To compile to C, do `python3.6 setup.py build_ext -i` in this directory. """

cdef log_level test
test = FATAL

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


def safe(api_method: typing.Callable):
    """
    A decorator that traps and logs Runtime-specific exceptions.

    This prevents exceptions from propogating up the call stack and
    interrupting the student code caller. For example, setting a nonexistent
    parameter should not prevent subsequent statements from executing.

    Because a safe API method returns ``None`` on an exception, student code
    may still fail to proceed.
    """
    @functools.wraps(api_method)
    def api_wrapper(*args, **kwargs):
        try:
            return api_method(*args, **kwargs)
        except RuntimeBaseException as exc:
            log_runtime(ERROR, str(exc))
    return api_wrapper


class Mode(enum.Enum):
    """
    The mode represents the execution state of the robot.

    Attributes:
        IDLE: Student code is not executing.
        AUTO: Autonomous mode. Gamepad usage is disallowed.
        TELEOP: Tele-operation mode. Gamepad usage is allowed.
        ESTOP: Emergency stop. Immediately triggers a Runtime exit.
    """
    IDLE = 'IDLE'
    AUTO = 'AUTO'
    TELEOP = 'TELEOP'
    ESTOP = 'ESTOP'


class Alliance(enum.Enum):
    """
    The alliance are the teams playing the game.

    Attributes:
        BLUE: The blue team.
        GOLD: The gold team.
    """
    BLUE = 'BLUE'
    GOLD = 'GOLD'


cdef class Actions:
    """ Miscellaneous asynchronous callables. """
    @staticmethod
    @safe
    def sleep(int seconds):
        """ Suspend execution of this action. """
        return


cdef class Match:
    """ The current match state. """
    alliance = None
    mode = Mode.IDLE

    def as_dict(self) -> dict:
        """ Export this match as a serializable dictionary. """
        return {'alliance': self.alliance, 'mode': self.mode}


cdef class DeviceAPI:
    """
    An API that exposes access to a collection of devices (gamepads or sensors).

    Attributes:
        device_buffers: A mapping of unique identifiers (not necessarily UIDs)
            to device buffers.
    """
     # Include device handler here

    def _get_device_buffer(self, char* device_uid):
        """ Retrieve a device buffer, or raise an exception if not found. """
        try:
            # return self.device_buffers[device_uid]
            pass
        except KeyError as exc:
            raise RuntimeExecutionError('Unrecognized device', device_uid=device_uid) from exc

    def _get_value(self, char* device_uid, char* param) -> typing.Any:
        """ Read a device parameter. """
        device = self._get_device_buffer(device_uid)
        try:
            # return device.struct.get_current(param)
            pass
        except AttributeError as exc:
            raise RuntimeExecutionError(
                'Unrecognized or unreadable parameter',
                device_uid=device_uid,
                device_type=type(device),
                param=param,
            ) from exc


cdef class Gamepad(DeviceAPI):
    """
    The API for accessing gamepads.

    Attributes:
        mode: The execution state of the robot.
    """
    mode = Mode.IDLE

    @safe
    def get_value(self, param: str, gamepad_id: int = 0):
        """
        Get a gamepad parameter if the robot is in teleop.

        Attributes:
            param: The name of the parameter to read.
            gamepad_id: The gamepad index (defaults to the first gamepad).
        """
        if self.mode is not Mode.TELEOP:
            raise RuntimeExecutionError(f'Cannot use Gamepad during {self.mode}',
                                        mode=self.mode, gamepad_id=gamepad_id,
                                        param=param)
        return super()._get_value(f'gamepad-{gamepad_id}', param)


cdef class Robot(DeviceAPI):
    """
    The API for accessing the robot and its sensors.

    Attributes:
        action_executor: An action executor.
        aliases: A device alias manager.
    """
    # Include device handler and executor API
    pass

    # @safe
    # def run(self, action: Action, *args, timeout: Real = 300):
    #     """ Schedule an action (coroutine function) for execution in a separate thread. """
    #     self.action_executor.register_action_threadsafe(action, *args, timeout=timeout)

    # @safe
    # def is_running(self, action: Action) -> bool:
    #     """ Check whether an action is currently executing. """
    #     self.action_executor.is_running(action)

    # def _normalize_device_uid(self, device_uid: typing.Union[str, int]) -> str:
    #     """ Translate device aliases into UIDs and return a device mapping key. """
    #     device_uid = str(device_uid)
    #     if device_uid in self.aliases:
    #         device_uid = self.aliases[device_uid]
    #     return f'smart-sensor-{device_uid}'

    # @safe
    # def get_value(self, device_uid: typing.Union[str, int], param: str) -> ParameterValue:
    #     """ Get a smart sensor parameter. """
    #     return super()._get_value(self._normalize_device_uid(device_uid), param)

    # @safe
    # def set_value(self, device_uid: typing.Union[str, int], param: str, value: ParameterValue):
    #     """ Set a smart sensor parameter. """
    #     device = self._get_device_buffer(self._normalize_device_uid(device_uid))
    #     try:
    #         device.struct.set_desired(param, value)
    #     except AttributeError as exc:
    #         raise RuntimeExecutionError(
    #             'Unrecognized or read-only parameter',
    #             device_uid=device_uid,
    #             device_type=type(device.struct).__name__,
    #             param=param,
    #         ) from exc


logger_init(EXECUTOR)
log_runtime(DEBUG, "Test Student API")
