# Student Code Executor

This folder includes the processes to understand the student API and execute the student code using the corresponding Runtime functions. 

## Building

### Dependencies:
This program will only work on Linux and MacOS systems. You need to have Python with version `>= 3.6`. The only Python package needed is `Cython` which can be installed with `python3 -m pip install Cython`. You also need to have `gcc` installed to compile the C code.

### Steps:
First, ensure that for whatever `python3.x` version you are using, there is a corresponding command line tool `python3.x-config`. On Linux, this can be installed by doing `sudo apt install python3.x-dev` with the `x` appropriately substituted with your version number. Then in the Makefile, change the `py` flag under the comment with the version of Python you are using.

To make the `studentapi`, do `make studentapi` which uses the `setup.py` file to compile the Cython extensions into C extensions and then into an executable file that can be imported in Python with `import studentapi`.

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

## Details

### Student API
The student API is written in Cython, which is a static compiler for Python. It provides static typing and lets you easily call C functions within Python. You can read the documentation at https://cython.readthedocs.io/en/latest/index.html. The specification for the API functions can be read
here https://pioneers.berkeley.edu/software/robot_api.html. 

In Cython, the `.pxd` files act as header files while `.pyx` files act as source files. In `studentapi.pxd`, you will see many extern declarations to C files. In `studentapi.pyx`, each API function converts the Python arguments into C arguments and calls the corresponding C functions to access shared memory. The only exception is `Robot.run` and `Robot.is_running` which are purely Python and use the Python `threading` module to run a robot action asynchronously to the main thread. 

### Executor Process
This process will read the current `mode` from the `shm_wrapper_aux` which will be supplied by the `NET_HANDLER`. Then whenever the `mode` changes to `AUTO` or `TELEOP`, the `main` function will spawn a new process to run the new mode, using `fork` and `run_mode`. If `mode` changes to `IDLE`, it will kill the previously running process using `kill_subprocess`. The `main` function will loop forever and is only cancellable by sending a SIGINT signal.

`run_mode` will call the `<mode>_setup` and the `<mode>_main` function using `run_py_function`. This functions will end up calling the Python C API located in `"Python.h"` to run the Python functions in the given student code, which is assumed to be in `studentcode.py`. More can be read about the Python C API at https://python.readthedocs.io/en/stable/c-api/index.html. 

To actually have the student code call the student API functions, we need to insert the API functions into the student module's namespace. This is done in `executor_init` where the logger, shared memory, and Python are all initialized. The insertion is done by setting the student code's attributes `Robot` and `Gamepad` to the corresponding attributes in the student API. 

The last mode is `CHALLENGE` which is used to run the student's coding challenges. The subprocess is started the same as the other modes but will call `run_challenges` instead. This function will start reading from the challenge UNIX socket that is listening to the `net_handler` for the challenge inputs. It then runs the challenges with its inputs using `run_py_function`. Finally, it will return the outputs as a string to the `net_handler` by sending over the challenge UNIX socket.

The tricky thing for the `CHALLENGE` mode is that it isn't idempotent like the other 2 modes since it requires an input. As a result, we need to ensure that if the `CHALLENGE` mode finishes or gets cancelled, it will set the mode to `IDLE` first.

One thing to note which can cause unwanted interactions is that the executor process is now linked dynamically, and so it shares the symbols for the shared memory and logger with the `studentapi.pyx` file. This means that if you initialize it once on `executor.c`, you don't need to initialize it in `studentapi.pyx`.

Another very important caveat to be aware of is Python's global interpreter lock (GIL). The Python interpreter can run in only 1 thread at a time and which thread is running is the one that acquires the GIL. As a result, whenever any Python C API functions need to be called, you must always be careful to acquire the GIL first and then release it when you are done. To make it easier on the programmer, we designed the architecture such that each mode runs in a separate process instead of a separate thread.

More details on how all these functions work are in the function documentation in `executor.c`.
