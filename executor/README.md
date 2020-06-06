# Student Code Executor

This folder includes the processes to understand the student code API, interpret it, and execute the corresponding Runtime functions. 

## Building

### Dependencies:
This program will only work on Linux and MacOS systems. You need to have Python with version `>= 3.6`. The only Python package needed is `Cython` which can be installed with ```python3 -m pip install Cython```. You also need to have `gcc` installed to compile the C code.

### Steps
First, ensure that for whatever `python3.x` version you are using, there is a corresponding command line tool `python3.x-config`. On Linux, this can be installed by doing `sudo apt install python3.x-dev` with the `x` appropriately substituted with your version number. Then in the Makefile, change the `py` flag under the comment with the version of Python you are using.

To make the `studentapi`, do `make studentapi` which uses `setup.py` file to compile the Cython extensions into C extensions and then into an executable file that can be imported in Python with `import studentapi`.

To make the executor, do `make executor` which uses `gcc` to compile the C code to an executable.

## Testing
To just test the student API functions, run `make test_api`. 

To test the executor, you first need to create instances of the `DEV_HANDLER` and the `NET_HANDLER`. This can be done by in one terminal going to `c-runtime/shm_wrapper`, then doing 
> make static  
> ./static  

In another terminal, you need to go to `c-runtime/shm_wrapper_aux` and then do
>make static  
> ./static  

In the third terminal, you can finally run `./executor` to test that the executor process properly spawns the threads to run the student code. The code that is by default ran will be in `studentcode.py`.

## Detailed Description

### Student API
The student API is written in Cython, which is a static compiler for Python. It provides static typing and lets you easily call C functions within Python. You can read the documentation at https://cython.readthedocs.io/en/latest/index.html. The specification for the API functions can be read
here https://pioneers.berkeley.edu/software/robot_api.html. 

In Cython, the `.pxd` files act as header files while `.pyx` files act as source files. In `studentapi.pxd`, you will see many extern declarations to C files. In `studentapi.pyx`, each API function converts the Python arguments into C arguments and calls the corresponding C functions to access shared memory. The only exception is `Robot.run` and `Robot.is_running` which are purely Python and use the Python `threading` module to run a robot action asynchronously to the main thread. 

One thing to note is that since the student API is compiled into a separate binary than where the executor is compiled to, they will have different memory spaces. Thus, the `studentapi` will access a different copy of the `shm_wrapper_aux` code and so the shared memory `init` and `stop` functions need to be appropriately run within the student API. Currently this is implemented by running SHM `init` in the class's `__cinit__` and running SHM `stop` in the class's `__dealloc__`.

### Executor Process
This process will read the current `mode` from the `shm_wrapper_aux` which will be supplied by the `NET_HANDLER`. Then whenever the `mode` changes to `AUTO` or `TELEOP`, the `main` function will spawn a thread to run the new mode, using `start_mode_thread`. If `mode` changes to `IDLE`, it will cancel the previously running thread. The `main` function will loop forever and is only cancellable by sending a SIGINT signal which then runs `executorStop`. This process heavily relies on the POSIX `pthread.h` library.

`start_mode_thread` will call the `setup` function using `run_function_timeout` and the `main` function using `run_function_loop`. Both of these functions will end up calling the Python C API located in `"Python.h"` to run the Python functions in the given student code, which is assumed to be in `studentcode.py`.

Details on the blocking nature of these functions are in the function's documentation.

