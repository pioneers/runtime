# Runtime Shared Memory Wrapper

The Runtime Shared Memory Wrapper provides an API for the device handler, network handler, and executor to use to communicate the state of devices, robot state, and gamepad state to each other, abstracting away the details of the underlying interprocess communication via shared memory. 

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

`print_shm.c` is another simple program that connects to the shared memory wrapper as the `TEST` process, calls some or all of the printing utility functions in the shared memory wrapper, then disconnects from the shared memory wrapper. It is useful when testing Runtime to view the state of the shared memory wrapper and calling the shared memory wrapper utilities from the processes being tested would be inconvenient. To compile this program, run
```
make print
```
Then do `./print`. This process should print some data to the terminal and return immediately.

## Detailed Description

### Motivation

Interprocess communication (IPC) is orders of magnitude of slower than communication between threads in the same process, which is slower still than communication between functions in the same thread. If we could, the fastest way to ferry data between Dawn/Shepherd, student code, and the devices attached to the robot would be to have the entirety of Runtime contained within a single thread of a single process. This, however, is obviously impractical for code readability, complexity, and maintainability, so Runtime is broken down into several independent processes (which themselves have separate threads) to divide up the tasks. As a result, we now need to deal with IPC.

There are various methods of IPC; in order of (generally) slowest to fastest: file sharing, sockets, pipes and message queues, and shared memory. Since so much of the data is being transferred over IPC, and previous iterations of runtime using pipes still had problems with the speed of the IPC, the decision was made to use shared memory for c-runtime's IPC. Additionally, shared memory is well-suited for Runtime since the data that is passed between the processes is very structured and consistent (device data, gamepad data, robot state), so we don't have to worry about constantly expanding or shrinking the size of the shared memory blocks.

### POSIX Shared Memory Overview

We use POSIX shared memory (as opposed to the older System V shared memory), because it allows us to work with names of files in the file system and do away with the keys that are needed in System V.

The way POSIX shared memory works is that one process first creates the shared memory block of a certain size, and with a predefined name (which is a path in the filesystem). This essentially "creates a file" in the filesystem. Then, the creating process maps the file into certain addresses in its own process space, which allows that process to write to that file simply by writing to locations in its own process space. Now, other processes on the same computer can "open" this newly created "file" and map it to their own process spaces, which results in a situation where writing to this special file from any process updates that file for all processes. When a process is terminated, it needs to ensure that its mappings to the shared memory are closed, and the process which created the shared memory file needs to unlink the name of that file from the file itself, so that the operating system can reclaim that space once there are no more references to that file. If this doesn't to happen, any new attemps to bind the same name to a newly created piece of shared memory will fail.

In the runtime shared memory wrapper, the process `shm.c` is responsible for creating and unlinking.

For more details and a great example of POSIX shared memory, see this page: https://www.softprayog.in/programming/interprocess-communication-using-posix-shared-memory-in-linux

### Additional Details

Shared memory almost always has to be used in conjunction with semaphores, because you want to prevent proceses from reading and writing the same block of memory at the same time. This is certainly the case in the shared memory wrappers.

#### Semaphores

A quick introduction to semaphores: a semaphore is basically a piece of memory owned by the operating system that holds a nonnegative integer. The semaphore is accessible to any process that knows its name. The operating system guarantees that any change to the value of this integer is "atomic", meaning that when a process increments or decrements the value of the semaphore, that value is reflected for all processes that have that semaphore open. It sounds a little pointless, but semaphores are actually very useful. Say, for example, that two processes are sharing some data. When one process tries to modify the data, the other process must not try to modify the data at the same time. It needs to wait until the first process is done updating the data before accessing it. Semaphores help ensure that this happens. You impose the following rule on both processes: if a process wants to modify the data, it needs to first decrement the semaphore before modifying the data; after it has finished modifying the data, it must then increment the semaphore. If you initialize the semaphore with the value of 1 at the start of the program, this ensures that no more than one process will be accessing the data at any given time (because remember that the value of the semaphore cannot be negative).

In the shared memory wrapper, each device that is registered in the shared memory wrapper has two associated semaphores: one for its `DATA` stream (where `dev_handler` writes device data for `executor` to read when executing student code), and another for its `COMMAND` stream (where `executor` writes commands from student code for `dev_handler` to pass to the devices). When any process wants to access the data on a particular stream by calling the shared memory wrapper, the wrapper will automatically acquire the associated semaphore before accessing the data, and release it after the access. There are also five additional semaphores. The first two, `cmd_map_sem` and `catalog_sem`, are useful for facilitating the fast transfer of commands from `executor` to `dev_handler`, and for connecting and disconnecting devices, respctively. The next two, `rd_sem` and `gp_sem`, are for access-guarding the robot description shared memory block and the gamepad state shared memory block, respectively. There is also an additional check to make sure that a gamepad is connected before attempting to access the gamepad state. The last, `sub_map_sem`, is useful for ensuring that the mechanism for placing and getting subscription requests on devices does not encounter race conditions.

#### The Command Bitmap

The fast transfer of commands from `executor` and `dev_handler` is done as follows. There is a command bitmap, which consists of `MAX_DEVICES + 1` elements that are each `MAX_PARAMS` bits long. If every single bit in the entire bitmap is 0, then no parameters on any devices in the system have been requested to change by the `executor`. The bits in the first element of the command bitmap correspond to which devices have had their parameters changed; the bits in each of the other elements of the command bitmap correspond to which params of a particular device have changed. This is best illustrated with an example. 
Suppose that both `MAX_DEVICES` and `MAX_PARAMS` were 32. Then the command bitmap would be made up of 33 elements, each element being a `uint32_t`. At the beginning, suppose a device connects to the shared memory wrapper and is assigned the index of 0. This device also has 3 parameters, at indices 0, 1, and 2. When no changes are requested on this device, we have the following state (with the least significant bit (the 0th bit) on the right):  
```
cmd_map[0]               cmd_map[1]               cmd_map[2]               .......    cmd_map[32]
<32 bits> ... 0000000    <32 bits> ... 0000000    <32 bits> ... 0000000    .......    <32 bits> ... 0000000
```
Now suppose `executor` is running some student code, and the student code has requested to change the param at index 1 of the connected device. When `executor` calls shared memory to write this new value to that device's `COMMAND` block, the wrapper takes note of this and sets the bit corresponding to that device to 1 in `cmd_map[0]`, and the bit corresponding to that device and that param to `1` (in this case, the second bit in `cmd_map[1]`). So after the `executor` writes to the `COMMAND` block and returns, we now have this state (again, with the least significant bit (the 0th bit) on the right):  
```
cmd_map[0]               cmd_map[1]               cmd_map[2]               .......    cmd_map[32]
<32 bits> ... 0000001    <32 bits> ... 0000010    <32 bits> ... 0000000     .......    <32 bits> ... 0000000
```
The 0th bit of `cmd_map[0]` being set to 1 says that the device at index 0 has one or more parameters changed. The 1st bit of `cmd_map[1]` being set to 1 says that the 1st parameter of the device at index 0 has had its value changed. 
This makes several optimizations possible for `dev_handler`. `dev_handler` no longer needs to read from each connected device continuously and compare it with the command that was most recently sent to the device to determine whether or not a new command was changed. Rather, it can just continuously read `cmd_map[0]` and check if it is equal to 0. If it is not equal to 0, that means that at least one device has new commands to send, and it can proceed to retrieve those new values by calling `device_read` on exactly the devices specified in `cmd_map[0]`. Furthermore, `dev_handler` can simply provide the bitmap corresponding to that device for which parameters have changed to the call to `device_read` as the argument `params_to_read`, meaning that the `dev_handler` does not waste any time retrieving, processing, packaging, or sending commands that have not changed. In our example, `dev_handler` would read `cmd_map[0]`, notice that the 0th bit is set to 1, and call `device_read` with `dev_ix = 0` and `params_to_read = cmd_map[1]` to retrieve the new param values.
Of course, after `dev_handler` retrieves the data, all of the read bits are cleared by the shared memory wrapper (set back to 0).

#### The Subscription Bitmaps

Lastly, an explanation of the purpose of the subscription bitmaps. A device such as the motor controller may have tens of parameters. It becomes computationally expensive for `dev_handler` to be reading in device data containing the current values of all of these parameters many times per second, when it may be the case that the student code never needs to read most of these parameters, or that the student is not viewing these parameters in Dawn. The solution is to have `dev_handler` _subscribe_ to only the parameters that are actually being viewed by the student in Dawn, and used by the student code running in `executor`. It becomes necessary to allow both `executor` and `net_handler` to communicate which parameters of which devices they would like the `dev_handler` to subscribe to.

Thus, we have the two internal bitmaps, `net_sub_map` and `exec_sub_map` ("`net_handler` subscription bitmap" and "`executor` subscription bitmap", respectively), that are arrays of `MAX_DEVICES + 1` bitmaps, each `MAX_PARAMS` long, that represent which parameters each process has requested a subscription from. At any given time, the subscription request on the device at index `i` in shared memory is given by the bitwise `OR` of the two elements `net_sub_map[i + 1]` and `exec_sub_map[i + 1]` i.e. the parameters that `dev_handler` needs to subscribe to from the device at index `i` is the union of the set of parameters requested by `net_handler` and the set of parameters requested by `executor`.

The two elements `net_sub_map[0]` and `exec_sub_map[0]` are special. The bit at position `i` in these elements represents whether the device at index `i` in shared memory has had its subscription changed since the last time `dev_handler` retrieved the subscription requests. That bit is turned on when `net_handler` or `executor` changes that device's subscription (it does not get turned on if those processes place a subscription request for the same parameters as what is currently being requested). This allows for several optimizations. When `dev_handler` retrieves the subscription requests by calling `get_sub_requests`, it need only check to see if the 0th element of the returned bitmap (`sub_map`) is `0`; if it is, then nothing has changed since the last time `get_sub_requests` was called, and no work needs to be done. If it isn't `0`, then `dev_handler` can look at which bits are turned on in `sub_map[0]` and send a new subscription request to each device `i` that has its bit turned on in `sub_map[0]`, and the params that it needs to subscribe to are given in `sub_map[i + 1]`. No further calculation, polling, or function calling must be done.

## Todos

1. Removing relative pathnames in the includes (fixable by adding directores with `-I` flag when compiling), issue #6 in Github
2. Clean up included headers (remove some that define the same functions, perhaps consider moving some to `runtime_util`, etc.)