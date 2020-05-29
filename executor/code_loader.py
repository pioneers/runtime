import time
import importlib
import sys
import signal
import studentapi
import typing

# The student code must be in the same directory as the API and this loader
class StudentCodeExecutor:

    setup_timeout = 5.0
    interval = 0.01
    TIMEOUT = {
        studentapi.AUTO: 30.0,
        studentapi.TELEOP: 120.0, 
    }
    
    def __init__(self, student_module: str):
        self.student_module = student_module
        self.load_code()

    def run_with_timeout(self, func: typing.Callable, timeout: float, *args, **kwargs):
        signal.setitimer(signal.ITIMER_REAL, timeout)
        try:
            return func(*args, **kwargs)
        finally:
            signal.setitimer(signal.ITIMER_REAL, 0)


    def load_code(self):
        self.student_code = importlib.import_module(self.student_module)
        def _print(*values, sep=' '):
            string = sep.join(map(str, values))
            studentapi.log_runtime(studentapi.INFO, string.encode('utf-8'))
        self.student_code.print = _print
        self.student_code.Robot = studentapi.Robot()
        self.student_code.Gamepad = studentapi.Gamepad()


    def get_function(self, mode, type: str):
        assert type in ['setup', 'main']
        if mode == studentapi.AUTO:
            name = f'autonomous_{type}'
        elif mode == studentapi.TELEOP:
            name = f'teleop_{type}'
        else:
            raise studentapi.RuntimeBaseException("Functions can only be called in autonomous or teleop mode.")
        
        try:
            func = getattr(self.student_code, name)
        except AttributeError:
            studentapi.log_runtime(studentapi.WARN, f'Unable to find function {name}, using stub'.encode('utf-8'))
            def func(): return None
        return func


    def start_mode(self, mode):
        setup = self.get_function(mode, 'setup')
        main = self.get_function(mode, 'main')
        signal.setitimer(signal.ITIMER_REAL, self.TIMEOUT[mode])
        try:
            self.run_with_timeout(setup, self.setup_timeout)
        except Exception as e:
            studentapi.log_runtime(studentapi.ERROR, f"Error in {mode} setup: {e}".encode('utf-8'))
            signal.setitimer(signal.ITIMER_REAL, 0)
            return -1

        try:
            while True:
                start = time.time()
                main()
                diff = time.time() - start
                time.sleep(self.interval - diff)
        except Exception as e:
            studentapi.log_runtime(studentapi.ERROR, f"Error in {mode} main: {e}".encode('utf-8'))
            return -1
        finally:
            signal.setitimer(signal.ITIMER_REAL, 0)
        return 0
        

# First argument is student file name, second argument is robot mode
if __name__ == '__main__':
    executor = StudentCodeExecutor(sys.argv[1])
    executor.start_mode(sys.argv[2])
