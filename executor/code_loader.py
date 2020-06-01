import time
import importlib
import sys
import signal
import studentapi
from studentapi import CodeExecutionError
import typing
import studentcode

def sigalarm_handler(signum, frame):
    raise TimeoutError

# signal.signal(signal.SIGALRM, sigalarm_handler)

# The student code must be in the same directory as the API and this loader
class StudentCodeExecutor:

    setup_timeout = 2.0
    interval = 0.01
    TIMEOUT = {
        'autonomous': 5.0,
        'teleop': 120.0, 
    }
    

    def __init__(self, student_module: str, mode: str):
        self.student_module = student_module
        self.mode = mode
        self.load_code()


    def run_with_timeout(self, func: typing.Callable, timeout: float, *args, **kwargs):
        signal.setitimer(signal.ITIMER_REAL, timeout)
        try:
            return func(*args, **kwargs)
        finally:
            signal.setitimer(signal.ITIMER_REAL, 0)


    def load_code(self):
        self.student_code = importlib.import_module(self.student_module)
        setattr(self.student_code, 'print', studentapi._print)
        setattr(self.student_code, 'Robot', studentapi.Robot())
        setattr(self.student_code, 'Gamepad', studentapi.Gamepad(self.mode))


    def get_function_safe(self, name: str):
        try:
            func = getattr(self.student_code, name)
        except AttributeError:
            studentapi.log_runtime(studentapi.WARN, f'Unable to find function {name}, using stub'.encode('utf-8'))
            def func(): return None
        return func


    def run(self):
        if self.mode != 'autonomous' and self.mode != 'teleop':
            raise studentapi.RuntimeBaseException("Only modes are autonomous and teleop.")
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
                time.sleep(self.interval - diff)
        except TimeoutError:
            log_runtime(DEBUG, "Got timeout error")
            pass
        except CodeExecutionError:
            pass
        except Exception as e:
            raise CodeExecutionError(f"Error in {self.mode} main: {repr(e)}")
        finally:
            signal.setitimer(signal.ITIMER_REAL, 0)
        

def run(file_name: str, mode: str):
    executor = StudentCodeExecutor(file_name, mode)
    executor.run()


# First argument is student file name, second argument is robot mode
if __name__ == '__main__':
    run(sys.argv[1], sys.argv[2])
