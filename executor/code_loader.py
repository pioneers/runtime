import time
import importlib
import sys
import signal
import studentapi
from studentapi import CodeExecutionError, _print
import typing
import studentcode


def sigalarm_handler(signum, frame):
    """Handler function that is called when process is interrupted by a SIGALRM. Causes process to cleanly exit."""
    raise TimeoutError

# To register the handler for the specified signal
signal.signal(signal.SIGALRM, sigalarm_handler)

# The student code must be in the same directory as the API and this loader
class StudentCodeExecutor:
    """Handles the running of student code in a separate process."""

    # Predefined timeouts to be used:
    setup_timeout = 2.0
    interval = 0.01
    TIMEOUT = {
        'autonomous': 5.0,
        'teleop': 120.0, 
    }
    

    def __init__(self, student_module: str, mode: str):
        """
        Initializes class attributes and loads the student code.
        Args:
            student_module: name of python file with student code, without the .py
            mode: either 'autonomous' or 'teleop'
        """
        self.student_module = student_module
        self.mode = mode
        self.load_code()


    def run_with_timeout(self, func: typing.Callable, timeout: float, *args, **kwargs):
        """Runs any function `func` for the specified amount of time `timeout`. """
        signal.setitimer(signal.ITIMER_REAL, timeout)
        try:
            return func(*args, **kwargs)
        finally:
            signal.setitimer(signal.ITIMER_REAL, 0)


    def load_code(self):
        """Loads student code into this file's namespace."""
        self.student_code = importlib.import_module(self.student_module)
        setattr(self.student_code, 'print', _print)
        setattr(self.student_code, 'Robot', studentapi.Robot())
        setattr(self.student_code, 'Gamepad', studentapi.Gamepad(self.mode))


    def get_function_safe(self, name: str):
        """Tries to get a function with name `name` from the student's module. If not found, returns a stub function that does nothing. """
        try:
            func = getattr(self.student_code, name)
        except AttributeError:
            _print(f'Unable to find function {name}, using stub')
            def func(): return
        return func


    def run(self):
        """Runs the student code with the predefined timeouts. First calls `mode_setup()` then calls `mode_main()` based on the `mode` attribute. The timeout is handled using SIGALRM signals."""
        if self.mode != 'autonomous' and self.mode != 'teleop':
            raise CodeExecutionError("Only modes that can be run are autonomous and teleop.")
        setup = self.get_function_safe(f'{self.mode}_setup')
        main = self.get_function_safe(f'{self.mode}_main')
        start = time.time()
        try:
            self.run_with_timeout(setup, self.setup_timeout)
        except Exception as e:
            signal.setitimer(signal.ITIMER_REAL, 0)
            raise CodeExecutionError(f"Error in {self.mode} setup: {repr(e)}")
        diff = time.time() - start

        signal.setitimer(signal.ITIMER_REAL, self.TIMEOUT[self.mode] - diff)
        try:
            while True:
                start = time.time()
                main()
                diff = time.time() - start
                if diff > self.interval:
                    raise CodeExecutionError(f"An iteration of {self.mode}_main took {diff} seconds, longer than {self.interval} seconds, indicating student code is stuck in a loop.")
                while((time.time() - start) < self.interval):
                    pass
        except TimeoutError:
            pass
        except CodeExecutionError:
            pass
        except Exception as e:
            raise CodeExecutionError(f"Error in {self.mode} main: {repr(e)}")
        finally:
            signal.setitimer(signal.ITIMER_REAL, 0)
        

def run(file_name: str, mode: str):
    "Helper function to be used by C executor. Same arguments as `StudentCodeExecutor` constructor."
    executor = StudentCodeExecutor(file_name, mode)
    executor.run()


""" First argument is student file name, second argument is robot mode. """
if __name__ == '__main__':
    run(sys.argv[1], sys.argv[2])
