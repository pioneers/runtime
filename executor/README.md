# Student Code Executor

This folder includes the processes to understand the student API and execute the student code using the corresponding Runtime functions.  Learn more about how the executor works in our [wiki](https://github.com/pioneers/runtime/wiki/Executor).

## Building

## Dependencies
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

## Building
First, ensure that for whatever `python3.x` version you are using, there is a corresponding command line tool `python3.x-config`. On Linux, this can be installed by doing `sudo apt install python3.x-dev` with the `x` appropriately substituted with your version number. Then in the Makefile, change the `py` flag under the comment with the version of Python you are using. You might also have to change the `Python.h` header in `executor.c` to use your Python version.

To make the `studentapi`, do `make studentapi` which uses the `setup.py` file to compile the Cython extensions into C extensions and then into an executable file that can be imported in Python with `import studentapi`.

To make the executor, do `make executor` which uses `gcc` to compile the C code to an executable.

To make everything, do `make`.

## Testing

To just test the student API functions, run `make test_api`. 

The recommended approach to testing is to use the test CLIs, which you can read more about in `tests/README.md`. 

### Without CLIs

To test the executor, you first need to create shared memory and then create instances of the `dev_handler` and the `net_handler`. This can be done by calling `./shm_start` in `shm_wrapper` then either creating the true processes, or spawning fake processes. The fake processes are located in `shm_wrapper/`.

Now, you can run `./executor <code file>` to spawn the executor process. The first argument is the name of the student code file that is run, without the `.py`. By default, the executor will run code in `studentcode.py`. Changing the run mode is done in the `net_handler` process.
