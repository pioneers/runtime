# Student Code Executor

This folder includes the processes to understand the student code API, interpret it, and execute the corresponding Runtime functions. 

## Building

### Dependencies:
This program will only work on Linux and MacOS systems. You need to have Python with version `>= 3.6`. The only Python package needed is `Cython` which can be installed with `python3 -m pip install Cython`. You also need to have `gcc` installed to compile the C code.

### Steps:
First, ensure that for whatever `python3.x` version you are using, there is a corresponding command line tool `python3.x-config`. On Linux, this can be installed by doing `sudo apt install python3.x-dev` with the `x` appropriately substituted with your version number. Then in the Makefile, change the `py` flag under the comment with the version of Python you are using.

To make the `studentapi`, do `make studentapi` which uses `setup.py` file to compile the Cython extensions into C extensions and then into an executable file that can be imported in Python with `import studentapi`.

To make the executor, do `make executor` which uses `gcc` to compile the C code to an executable.

## Testing
First, follow the build instructions. To just test the student API functions, run `make test_api`. 

To test the executor, you first need to create instances of the `DEV_HANDLER` and the `NET_HANDLER`. This can be done by first making the test files:

> cd shm_wrapper  
> make static  
> cd ../shm_wrapper_aux  
> make static

Then in a separate terminal that is at `c-runtime/`, do `./create_static_shm.sh`. This will begin the initial modes and afterwards, will be a prompt that waits for you to input the next mode of either `auto`, `teleop`, `idle`, or `stop`.

In the original terminal, you can finally go to `c-runtime/executor` and run `./executor` to test that the executor process properly spawns the threads to run the student code. The student code that is by default ran will be in `studentcode.py`. 

## Detailed Description

### Student API
The student API is written in Cython, which is a static compiler for Python. It provides static typing and lets you easily call C functions within Python. You can read the documentation at https://cython.readthedocs.io/en/latest/index.html. The specification for the API functions can be read
here https://pioneers.berkeley.edu/software/robot_api.html. 

In Cython, the `.pxd` files act as header files while `.pyx` files act as source files. In `studentapi.pxd`, you will see many extern declarations to C files. In `studentapi.pyx`, each API function converts the Python arguments into C arguments and calls the corresponding C functions to access shared memory. The only exception is `Robot.run` and `Robot.is_running` which are purely Python and use the Python `threading` module to run a robot action asynchronously to the main thread. 

### Executor Process
This process will read the current `mode` from the `shm_wrapper_aux` which will be supplied by the `NET_HANDLER`. Then whenever the `mode` changes to `AUTO` or `TELEOP`, the `main` function will spawn a thread to run the new mode, using `run_mode`. If `mode` changes to `IDLE`, it will cancel the previously running thread and safely stop the running Python threads using `stop_mode`. The `main` function will loop forever and is only cancellable by sending a SIGINT signal which then runs `executor_stop`. This process relies on the POSIX `pthread.h` library to create and manage all threads.

`run_mode` will call the `<mode>_setup` and the `<mode>_main` function using `run_py_function`. This functions will end up calling the Python C API located in `"Python.h"` to run the Python functions in the given student code, which is assumed to be in `studentcode.py`. More can be read about the Python C API at https://python.readthedocs.io/en/stable/c-api/index.html. 

To actually have the student code call the student API functions, we need to insert the API functions into the student module's namespace. This is done in `executor_init` where the logger, shared memory, and Python are all initialized. The insertion is done by setting the student code's attributes `Robot`, `Gamepad`, and `print` to the corresponding attributes in the student API. 

One thing to note is that the executor process is now linked dynamically, and so it shares the symbols for the shared memory and logger with the `studentapi.pyx` file. 

Another very important caveat to always consider is Python's global interpreter lock (GIL). The Python interpreter can run in only 1 thread at a time and which thread is running is the one that acquires the GIL. As a result, whenever ANY Python C API functions need to be called, we must always be careful to acquire the GIL first and then release it when we are done. More details on this can be read here https://python.readthedocs.io/en/stable/c-api/init.html#thread-state-and-the-global-interpreter-lock. 

More details on how all these functions work are in the function documentation in `executor.c`.