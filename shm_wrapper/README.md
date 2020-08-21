# Shared Memory Wrapper

The Shared Memory Wrapper provides an API for the device handler, network handler, and executor to use to communicate the state of devices, robot state, and gamepad state to each other, abstracting away the details of the underlying interprocess communication via shared memory. 

For more information regarding the shared memory wrapper, see [the Wiki page](https://github.com/pioneers/c-runtime/wiki/Shared-Memory-Wrapper).

## Contents

`shm_wrapper.h` contains the header file that should be included in each of the processes that use the wrapper. Please read the extensive comments in this file for an overview of the wrapper's usage. Source code for the wrapper is found in `shm_wrapper.c`. These files cannot be compiled or run by themselves; rather, they should be included by the other processes that wish to use it and compiled with those processes.

`shm_start.c` is the process that is responsible for creating and initializing all of the semaphores and shared memory blocks that are used by the other Runtime processes; `shm_stop.c` is the process that is responsible for unlinking and destroying all of the shared memory blocks and semaphores. By giving the job of creating and unlinking the shared memory blocks and semaphores to these two simple and thus very robust process, it ensures that even if any Runtime process crashes unexpectedly and `systemd` shuts down the processes in some random order, the shared memory blocks will be unlinked upon Runtime shutdown, thus preventing segmentation faults or other errors upon Runtime restart. To compile, run
```
make shm_start
make shm_stop
```
Then do `./shm_start` to create the shared memory blocks and semaphores. The process will exit in a short amount of time. Now you can run any of the Runtime processes in whichever order and it will boot up properly. When you are finished, run `./shm_stop` to clear out the shared memory blocks.

`shm_wrapper_test1.c` and `shm_wrapper_test2.c` contain a suite of tests to demonstrate that the wrapper is functioning properly. Run
```
make test1
make test2
make shm_start
make shm_stop
```
Then run by opening two separate terminal windows and running first `./shm_start` in a window (doesn't matter which one), then `./test1` and `./test2` in separate windows. It doesn't matter which of `./test1` or `./test2` you run first. The tests will run automatically to completion and print many status messages and updates to standard output. When you are finished, run `./shm_stop` to clear shared memory from your system.

## Testing

To view the contents of shared memory and manually poke the system, use the Runtime CLIs, found in `runtime/tests/cli`.