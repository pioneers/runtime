# Runtime Auxiliary Shared Memory Wrapper

The Runtime Shared Memory Wrapper provides an API for the device handler, network handler, and executor to use to communicate robot state and gamepad state to each other, abstracting away the details of the underlying interprocess communication via shared memory. 

## Contents

`shm_wrapper_aux.h` contains the header file that should be included in each of the processes that use the wrapper. Please read the extensive comments in this file for an overview of the wrapper's usage. Source code for the wrapper is found in `shm_wrapper_aux.c`. These files cannot be compiled or run by themselves; rather, they should be included by the other processes that wish to use it and compiled with those processes.

`shm_wrapper_aux_test1.c` and `shm_wrapper_aux_test2.c` contain a suite of tests to demonstrate that the auxiliary wrapper is functioning properly. Run
```
make test1
make test2
```
Then run by opening two separate terminal windows and running first `./test1` in one window and then `./test2` in the other. The tests will run automatically to completion and print many status messages and updates to standard output. 

`connect_static_aux.c` is a simple program that is, in some sense, a fake `net_handler`, in that it creates the shared memory blocks and keeps the blocks open at some static value. This program is necessary if we want to test any other process that uses the auxiliary shared memory wrapper by itself, without having to start up the full network handler process. Without it, the process we want to test would generate a segmentation fault (due to the shared memory not existing). To compile this program, run
```
make static
```
Then do `./static`. This process will hang forever until it receives `SIGINT` (`Ctrl-C`).

`print_aux_shm.c` is another simple program that connects to the auxiliary shared memory wrapper as the `TEST` process, calls some or all of the printing utility functions in the auxiliary shared memory wrapper, then disconnects from the auxiliary shared memory wrapper. It is useful when testing Runtime to view the state of the auxiliary shared memory wrapper and calling the auxiliary shared memory wrapper utilities from the processes being tested would be inconvenient. To compile this program, run
```
make print
```
Then do `./print`. This process should print some data to the terminal and return immediately.

## Detailed Description

### Motivation

See the corresponding section in the README for the shared memory wrapper.

### POSIIX Shared Memory Overview

See the corresponding section in the README for the shared memory wrapper.

### Additional Details

Shared memory almost always has to be used in conjunction with semaphores, because you want to prevent proceses from reading and writing the same block of memory at the same time. This is certainly the case in the shared memory wrappers. 

In the auxiliary shared memory wrapper, there are two semaphores: one for the robot description shared memory block, and another for the gamepad state shared memory block. Like the shared memory wrapper, the auxiliary wrapper will also automatically acquire and release the semaphores for the calling process. It also includes an additional check to make sure that a gamepad is connected before attempting to access the gamepad state. Lastly, due to a potential bug found during the development of the executor, it has some global variables that keep track of the state of both semaphores, which are used to ensure that a process does not terminate with a semaphore locked.

## Todos

1. Removing relative pathnames in the includes (fixable by adding directores with `-I` flag when compiling), issue #6 in Github
2. Clean up included headers (remove some that define the same functions, perhaps consider moving some to `runtime_util`, etc.)
3. Look into combining the two shared memory wrappers into one
4. Investigate or try to reproduce the bug where the executor would terminate without releasing `rd_sem`, issue #15 in Github