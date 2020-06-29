# Runtime Shared Memory Wrapper

The Runtime Shared Memory Wrapper provides an API for the device handler, network handler, and executor to use to communicate the state of devices to each other, abstracting away the details of the underlying interprocess communication via shared memory. 

## Contents

`shm_wrapper.h` contains the header file that should be included in each of the processes that use the wrapper. Please read the extensive comments in this file for an overview of the wrapper's usage. Source code for the wrapper is found in `shm_wrapper.c`. These files cannot be compiled or run by themselves; rather, they should be included by the other processes that wish to use it and compiled with those processes.

`shm_wrapper_test1.c` and `shm_wrapper_test2.c` contain a suite of tests to demonstrate that the wrapper is functioning properly. Run
```
make test1
make test2
```
Then run by opening two separate terminal windows and running first `./test1` in one window and then `./test2` in the other. The tests will run automatically to completion and print many status messages and updates to standard output. 

`connect_static_devices.c` is a simple program that is, in some sense, a fake `dev_handler`, in that it creates the shared memory blocks and keeps the blocks open at some static value. This program is necessary if we want to test any other process that uses the shared memory wrapper by itself, without having to start up the full device handler process. Without it, the process we want to test would generate a segmentation fault (due to the shared memory not existing). To compile this program, run
```
make static
```
Then do `./static`. This process will hang forever until it receives `SIGINT` (`Ctrl-C`).

`print_shm.c` is another simple program that connects to the shared memory wrapper as the `TEST` process, calls some or all of the printing utility functions in the shared memory wrapper, then disconnects from the shared memory wrapper. It is useful when testing Runtime to view the state of the shared memory wrapper and calling the shared memory wrapper utilities from the processes being tested would be inconvenient. To compile this program, run
```
make print
```
Then do `./print`. This process should print some data to the terminal and return immediately.

## Detailed Description

### Motivation

Interprocess communication (IPC) is orders of magnitude of slower than communication between threads in the same process, which is slower still than communication between functions in the same thread. If we could, the fastest way to ferry data between Dawn/Shepherd, student code, and the devices attached to the robot would be to have the entirety of Runtime contained within a single thread of a single process. This, however, is obviously impractical for code readability, complexity, and maintaineability, so Runtime is broken down into several independent processes (which themselves have separate threads) to divide up the tasks. As a result, we now need to deal with IPC.

There are various methods of IPC; in order of (generally) slowest to fastest: file sharing, sockets, pipes and message queues, and shared memory. Since so much of the data is being transferred over IPC, and previous iterations of runtime using pipes still had problems with the speed of the IPC, the decision was made to use shared memory for c-runtime's IPC. Additionally, shared memory is well-suited for Runtime since the data that is passed between the processes is very structured and consistent (device data, gamepad data, robot state), so we don't have to worry about constantly expanding or shrinking the size of the shared memory blocks.

### POSIX Shared Memory Overview

We use POSIX shared memory (as opposed to the older System V shared memory), because it allows us to work with names of files in the file system and do away with the keys that are needed in System V.

The way POSIX shared memory works is that one process first creates the shared memory block of a certain size, and with a predefined name (which is a path in the filesystem). This essentially "creates a file" in the filesystem. Then, the creating process maps the file into certain addresses in its own process space, which allows that process that write to that file simply by writing to locations in its own process space. Now, other processes on the same computer can "open" this newly created "file" and map it to their own process spaces, which results in a situation where writing to this special file from any process updates that file for all processes. When a process is terminated, it needs to ensure that its mappings to the shared memory are closed, and the process which created the shared memory file needs to unlink the name of that file from the file itself, so that the operating system can reclaim that space once there are no more references to that file. If this doesn't to happen, any new attemps to bind the same name to a newly created piece of shared memory will fail.

In the runtime shared memory wrapper, the `dev_handler` is responsible for creating and unlinking. For the auxiliary shared memory wrapper, it is the `net_handler` that is responsible.

For more details and a great example of POSIX shared memory, see this page: https://www.softprayog.in/programming/interprocess-communication-using-posix-shared-memory-in-linux

### Additional Details

Shared memory almost always has to be used in conjunction with semaphores, because you want to prevent proceses from reading and writing the same block of memory at the same time. This is certainly the case in the shared memory wrappers. 

In the shared memory wrapper, for each device that is registered in the shared memory wrapper, there are two semaphores: one for the `DATA` stream (where `dev_handler` writes device data for `executor` to read when executing student code), and another for the `COMMAND` stream (where `executor` writes commands from student code for `dev_handler` to pass to the devices). When any process wants to access the data on a particular stream by calling the shared memory wrapper, the wrapper will automatically acquire the associated semaphore before accessing the data, and release it after the access. There are also two additional semaphores, the `pmap_sem` and `catalog_sem` which are useful for facilitating the fast transfer of commands from `executor` to `dev_handler`, and for connecting and disconnecting devices, respctively.

The fast transfer of commands from `executor` and `dev_handler` is done as follows. There is a param bitmap, which consists of `MAX_DEVICES + 1` elements that are each `MAX_PARAMS` bits long. If every single bit in the entire bitmap is 0, then no parameters on any devices in the system have been requested to change by the `executor`. The bits in the first element of the param bitmap correspond to which devices have had their parameters changed; the bits in each of the other elements of the param bitmap correspond to which params of a particular device have changed. This is best illustrated with an example. 
Suppose that both `MAX_DEVICES` and `MAX_PARAMS` were 32. Then the param bitmap would be made up of 33 elements, each element being a `uint32_t`. At the beginning, suppose a device connects to the shared memory wrapper and is assigned the index of 0. This device also has 3 parameters, at indices 0, 1, and 2. When no changes are requested on this device, we have the following state (with 0th bit on the left for each element of `pmap`):  
```
pmap[0]                  pmap[1]                  pmap[2]                    ....... pmap[32]
00000000... <32 bits>    00000000... <32 bits?>   00000000... <32 bits> .... ....... 00000000...<32 bits>
```
Now suppose the `executor` is running some student code, and the student code has requested to change the param at index 1 of the connected device. When `executor` calls shared memory to write this new value to that device's `COMMAND` block, the wrapper also takes note of this and sets the bit corresponding to that device to 1 in `pmap[0]`, and the bit corresponding to that device and that param to `1` (in this case, the second bit in `pmap[1]`). So after the `executor` writes to the `COMMAND` block and returns, we now have this state (again, with the 0th bit on the left for each element of `pmap`):
```
pmap[0]                  pmap[1]                  pmap[2]                    ....... pmap[32]
10000000... <32 bits>    01000000... <32 bits?>   00000000... <32 bits> .... ....... 00000000...<32 bits>
```
The 0th bit of `pmap[0]` being set to 1 says that the device at index 0 has had parameters changed. The 1st bit of `pmap[1]` being set to 1 says that the 1st parameter of the device at index 0 has had its parameters changed. 
This allows for several optimizations on the `dev_handler` side. `dev_handler` no longer needs to read from each connected device continuously and compare it with the command that was most recently sent to the device to determine whether or not a new command was changed. Rather, it can just continuously read `pmap[0]` and check if that element is 0. If it is not 0, that means that at least one device has new commands to send, and it can proceed to retrieve those new values by calling `device_read` on exactly the devices specified in `pmap[0]`. Furthermore, `dev_handler` can simply provide the bitmap corresponding to that device for which parameters have changed to the call to `device_read` as the argument `params_to_read`, meaning that the `dev_handler` does not waste any time retriving, processing, packaging, or sending commands that have not changed. In our example, `dev_handler` would read `pmap[0]`, notice that the 0th bit is set to 1, and call `device_read` with `dev_ix = 0` and `params_to_read = pmap[1]` to retrieve the new param values.
Of course, after `dev_handler` retrieves the data, all of the read bits are cleared (set back to 0).

## Todos

1. Removing relative pathnames in the includes (fixable by adding directores with `-I` flag when compiling), issue #6 in Github
2. Clean up included headers (remove some that define the same functions, perhaps consider moving some to `runtime_util`, etc.)
3. Look into combining the two shared memory wrappers into one