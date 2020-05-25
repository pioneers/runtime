# Runtime Shared Memory Wrapper

The Runtime Shared Memory Wrapper provides an API for the device handler, network handler, and executor to use to communicate the state of devices to each other, abstracting away the details of the underlying interprocess communication via shared memory. 

## Description

`shm_wrapper.h` contains the header file that should be included in each of the processes that use the wrapper. Please read the extensive comments in this file for an overview of the wrapper's usage. Source code for the wrapper is found in `shm_wrapper.c`.

`shm_mmap_read.c` and `shm_mmap_write.c` provide a demo of the shared memory mechanism that the wrapper is based around. You may need to change the file path of the log file in order to run the test, found on line 29 of `shm_mmap_write.c`. Compile with
```bash
gcc shm_mmap_read.c -o shm_mmap_read
gcc shm_mmap_write.c -o shm_mmap_write
```
Then run by opening two separate terminal windows and running `./shm_mmap_read` in one window and `./shm_mmap_write` in the other. Enter in 10 messages into the terminal for `./shm_mmap_read` to complete the demo.

`shm_wrapper_tester1.c` and `shm_wrapper_tester2.c` contain a suite of tests to demonstrate that the wrapper is functioning properly. If using a Mac, run:
```bash
make tester1
make tester2
```
Then run by opening two separate terminal windows and running first `./tester1` in one window and then `./tester2` in the other. The tests will run automatically to completion and print many status messages and updates to standard output. 
If using a Linux system or Linux emulator on a Windows system, you may need to change the commands in the `Makefile` to:
```bash
gcc shm_wrapper_tester1.c shm_wrapper.c -o tester1 -lrt -lpthread
```
and similarly for `tester2`.

## Todos

We may potentially need more functionality to support the Gamepad device, and to store robot state (running mode, E-stop, etc.)

More tests could always be used; expanding on the existing testers is probably a good idea.