# Student Code Executor

This folder includes the processes to understand the student API and execute the student code using the corresponding Runtime functions. 

## Building

### Dependencies:
This program will only work on Linux systems. You need to have `gcc` installed to compile the C code.

For the Python code, you need Python with version `>= 3.6` and with the `Python.h` development header. You also need to have `pip` installed. This can be done with

    sudo apt-get install python3-dev
    sudo apt-get install python3-pip

To then install the only third-party Python dependency, do

    python3 -m pip install Cython

Runtime also uses the following language-supported Python libraries to implement the Student API:

* `threading`: used for creating threads that run student-defined actions
* `sys`: used for redirecting Python errors to the Runtime `logger`
* `builtins`: used to change the built-in Python `print` function and redirect it to the Runtime `logger`
* `importlib`: used by the code parser to import the student code dynamically
* `re`: used by the code parser to parse the student code with regex

### Steps:
First, ensure that for whatever `python3.x` version you are using, there is a corresponding command line tool `python3.x-config`. On Linux, this can be installed by doing `sudo apt install python3.x-dev` with the `x` appropriately substituted with your version number. Then in the Makefile, change the `py` flag under the comment with the version of Python you are using.

To make the `studentapi`, do `make studentapi` which uses the `setup.py` file to compile the Cython extensions into C extensions and then into an executable file that can be imported in Python with `import studentapi`.

To make the executor, do `make executor` which uses `gcc` to compile the C code to an executable.

To make everything, do `make`.

## Testing

First, follow the build instructions. To just test the student API functions, run `make test_api`. 

To test the executor, you first need to create instances of the `dev_handler` and the `net_handler`. This can be done by creating the true processes, or spawning fake processes. The fake processes are located in `shm_wrapper/`.

Now, you can run `./executor` to test that the executor process properly spawns the processes to run the student code. The student code that is by default ran will be in `studentcode.py`. Changing the mode is done on the `net_handler` process.

## Details

### Student API
The student API is written in Cython, which is a static compiler for Python. It provides static typing and lets you easily call C functions within Python. You can read the documentation at https://cython.readthedocs.io/en/latest/index.html. The specification for the API functions can be read
here https://pioneers.berkeley.edu/software/robot_api.html. 

In Cython, the `.pxd` files act as header files while `.pyx` files act as source files. In `studentapi.pxd`, you will see many extern declarations to C files. In `studentapi.pyx`, each API function converts the Python arguments into C arguments and calls the corresponding C functions to access shared memory. The only exception is `Robot.run` and `Robot.is_running` which are purely Python and use the Python `threading` module to run a robot action asynchronously to the main thread. 

### Executor Process
This process will read the current `mode` from the `shm_wrapper_aux` which will be supplied by the `net_handler`. Then whenever the `mode` changes to `AUTO` or `TELEOP`, the `main` function will create a new subprocess to run the new mode, using `fork` and `run_mode`. If `mode` changes to `IDLE`, it will kill the previously running process using `kill_subprocess`. To avoid any robot safety concerns, we must also reset the robot parameters on `IDLE`, which is done with `reset_params`. The `main` function handling the mode changes will loop forever and is only cancellable by sending a SIGINT signal.

To actually have the student code call the student API functions, we need to insert the API functions into the student module's namespace. This is done in `executor_init` where the Python interpreter is initialized. The insertion is done by setting the student code's attributes `Robot` and `Gamepad` to the corresponding attributes in the student API. 

Before the mode actually begins, we need to first send device subscription requests to the `dev_handler` to ensure that SHM is being updated with device data. To do this, we must parse the student code to see which devices and parameters they are using, then send the corresponding requests. These functions are in `code_parser.pyx` which is included in the `studentapi.pyx` namespace using the Cython `include` statement. Then in `executor_init`, we call `make_device_subs` from `code_parser.pyx`.

`run_mode` will call the `<mode>_setup` and the `<mode>_main` Python functions using `run_py_function`. These functions will end up calling the Python C API located in `"Python.h"` to run the Python functions in the given student code, which is assumed to be in `studentcode.py`. More can be read about the Python C API at https://python.readthedocs.io/en/stable/c-api/index.html. 

The last mode is `CHALLENGE` which is used to run the student's coding challenges. The subprocess is started the same as the other modes but will call `run_challenges` instead of `run_mode`. This function will start reading from the challenge UNIX socket that is listening to the `net_handler` for the challenge inputs. It then runs the challenges with its inputs using `run_py_function`. Finally, it will return the outputs as a string to the `net_handler` by sending over the challenge UNIX socket.

The tricky thing for the `CHALLENGE` mode is that it isn't idempotent like the other 2 modes since it requires an input. As a result, we need to ensure that if the `CHALLENGE` mode finishes or gets cancelled, it will set the mode to `IDLE` on exit.

One thing to note which can cause unwanted interactions is that the executor process is now linked dynamically, and so it shares the symbols for the shared memory and logger with the `studentapi.pyx` file. This means that if you initialize it once on `executor.c`, you don't need to initialize it in `studentapi.pyx`.

Another important caveat to be aware of is Python's global interpreter lock (GIL). The Python interpreter can run in only 1 thread at a time and which thread is running is the one that acquires the GIL. As a result, whenever any Python C API functions need to be called, you must always be careful to acquire the GIL first and then release it when you are done. To make it simpler, we designed the architecture such that each mode runs in a separate process instead of a separate thread, which avoids any issue with the Python interpreter state.

More details on how all these functions work are in the function documentation in `executor.c`.
