# Shared Memory Wrapper

The Shared Memory Wrapper provides an API for the device handler, network handler, and executor to use to communicate the state of devices, robot state, and gamepad state to each other, abstracting away the details of the underlying interprocess communication via shared memory. 

For more information regarding the shared memory wrapper, see [the Wiki page](https://github.com/pioneers/c-runtime/wiki/Shared-Memory-Wrapper).

## Contents

`shm_wrapper.h` contains the header file that should be included in each of the processes that use the wrapper. Please read the extensive comments in this file for an overview of the wrapper's usage. Source code for the wrapper is found in `shm_wrapper.c`. These files cannot be compiled or run by themselves; rather, they should be included by the other processes that wish to use it and compiled with those processes.

`shm.c` is the process that is responsible for creating and unlinking all of the semaphores and shared memory blocks that are used by the other Runtime processes. By giving the job of creating and unlinking the shared memory blocks and semaphores to this simple and thus very robust process, it ensures that even if any Runtime process crashes unexpectedly and `systemd` shuts down the processes in some random order, the shared memory blocks will be unlinked upon Runtime shutdown, thus preventing segmentation faults or other errors upon Runtime restart. To compile, run
```
make shm
```
Then do `./shm` to run. The process will hang until it receives `SIGINT` (`Ctrl-C`).

`shm_wrapper_test1.c` and `shm_wrapper_test2.c` contain a suite of tests to demonstrate that the wrapper is functioning properly. Run
```
make test1
make test2
make shm
```
Then run by opening three separate terminal windows and running first `./shm` in one window, then `./test1` in a second window and then `./test2` in the third window. It doesn't matter which of `./test1` or `./test2` you run first. The tests will run automatically to completion and print many status messages and updates to standard output. 

`static_dev.c` and `static_aux.c` are simple programs that are, in some sense, a fake `dev_handler` and a fake `net_handler`, respectively. They attach to the shared memory blocks and write static values to the device shared memory blocks and the robot description + gamepad state shared memory blocks, respectively, and then hang. These programs are useful if we want to test any other process that uses the shared memory wrapper by itself, without having to start up the corresponding full processes. Running `./shm` by itself sets all values in all shared memory blocks to 0, which isn't very interesting if we want to verify that certain processes are working as expected. To run these programs, first run
```
make static_dev
make static_aux
```
Then do `./static_dev` or `./static_aux`, depending on which blocks you would like to hold at static values. These processes will run until they receive `SIGINT` (`Ctrl-C`).

## Testing

To view the contents of shared memory and manually poke the system, use the Runtime CLIs, found in `c-runtime/tests/cli`.