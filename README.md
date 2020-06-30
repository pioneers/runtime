# PiE C-Runtime

Welcome to the PiE Runtime repo! If you're new a staff member, then welcome to PiE, and welcome to the Runtime project! We're so glad to have you with us, and that you chose to give Runtime a shot! This README will (hopefully) be a nice overview of what Runtime is all about and how it's built, and answer some basic questions you might have about our project and this code. If you have any questions, please ask the Runtime project manager(s)! We highly encourage you to read through the whole thing, it'll be really useful, we promise! Without further ado, let's get started...

## What is Runtime?

As you know, Pioneers in Engineering is a student organizations that hosts a robotics competition for underserved high schools in the SF Bay Area. To do this, we build everything in house, including our software. So, when the students get their robotics kits to build their robots, it includes motors, sensors, servos, gears, metal, and a computer to actually run their robot. **Runtime is responsible for writing the software that runs on that computer (a Raspberry Pi, at the time of this writing), as well as the software that runs on the Arduinos attached to all the motors, sensors, and servos.**

Students also write their own code to control their robots, which they upload to the robot for it to run. Most of their code consists of things like "when the Y button on the gamepad is pressed, I want to move this servo to this position" which does something that scores points. **The code that Runtime writes for the Raspberry Pi needs to be able to run the student's code and translate that into actual commands to the sensors on the robots.**

Students use the "Dawn" computer application (also made by PiE) running on their laptops to write their code, connect their Xbox controllers (we call them "gamepads" in Runtime) to, and view the current state of the various devices attached to their robot. **Runtime's software must be able to communicate with Dawn to receive commands and student code, and report device data back to Dawn.**

During the competition, the software "Shepherd" (another PiE software project) is used to control the field and run the game. Shepherd connects to the robots in order to tell them which side of the field they're on, where they're robots are starting, and when to start autonomous (auto) mode, when to start teleoperated (teleop) mode, and when to stop. **Runtime's software must be able to communicate with Shepherd to receive these commands and report any status updates back to Shepherd.**

The devices that are attached to the robot (motors, servos, sensors) must, of course, interact with the software on the Raspberry Pi. When the student's code wants to command a motor or servo to a new speed or position, it needs to send that information to the Arduino on the device that is controlling it. When the student's code wants to read the value of a sensor, the software on the Raspberry Pi must make that information available to it. And, the software on the Raspberry Pi must be able to detect when new devices have connected, or devices have been disconnected (unplugged) from the robot. **Runtime is responsible for ensuring all of this behavior.**

So, as you can see, Runtime does a lot! But, how does it all work?

## Design Principles

**Runtime is all about speed and reliability.** 

It needs to be fast, so that student code doesn't lag, so that inputs from the gamepads don't lag, and so that it can handle as many devices as the students would ever put onto their robot without crashing or noticeable slowing down of the system. This guided a lot of our early design decisions, and is part of the reason why we chose to write the entire system in C. <sup id="return1">[1](#footnote1)</sup>

Runtime also needs to be easy to debug and very robust, because nothing frustrates a student more than when the robot's software breaks while they're working, and it has nothing to do with their code. They get even more frustrated when they bring the broken robot to PiE staff members, who then take hours to try and solve the problem because the staff members can't debug the problem efficiently. 

Lastly, Runtime should be as easy to understand, set up, and maintain as possible, because the last thing we want is for the learning curve for staff to become so steep that it becomes impossible to keep the code stable, simple, and healthy. 

With that said, we now see the need for the following design principles:

* **Keep It Simple, Stupid (KISS)**. In Runtime, this means that we want to keep the number of dependencies on third-party libraries to a minimum. This reduces the complexity of building Runtime. Third party-libraries also often have incomplete or questionable documentation; having fewer third-party dependencies means that there is less documentation new staff members have to read in order to begin understanding and working on Runtime. 
* **Document, document, document**. As you can probably tell by the length and detail in this README, we take our documentation seriously. Mark your TODO items. Describe your functions. Name things well. Write and update READMEs whenever possible. We need to document everything to make it easy for staff to onboard, and for seasoned staff to maintain and extend the code.
* **Be Able to Log Anywhere**. Runtime has a logger that can send logs from anywhere within the system to a file, terminal, or over the network to Dawn if Dawn is connected. It is crucial that this logger be maintained, and for log statements to be added into the program wherever necessary (including in the Arduino devices). When a problem arises, staff must be able to turn on these logs and be able to see exactly what is happening in the system from somewhere, which enables efficient debugging.
* **Consider All Posssibilities**. Try to consider all the errors and situations possible. What should happen in each case? Should Runtime restart? Should it handle the error and continue as normal? Should it revert to some default behavior? This all helps make Runtime as robust as possible, helps us debug efficiently when Runtime crashes, and helps keep students happy when Runtime doesn't crash.
* **Test, test, test**. It is hard to test such a large system, but if at all possible, the effort must be made to test our code. 

## Overall Structure

Runtime can be divided into a few neatly containerized parts:

* **The Network Handler**: abbreviated `net_handler`, this is a process in Runtime, and is responsible for all communication between the Raspberry Pi and Dawn, and between the Raspberry Pi and Shepherd.
* **The Device Handler**: abbreviated `dev_handler`, this is a process in Runtime, and is responsible for all communication between the Raspberry Pi and all the devices that are attached to the robot at any given time.
* **The Executor**: this is a process in Runtime, and is responsible for running the student code that is uploaded to the robot from Dawn.
* **The Shared Memory Wrappers**: abbreviated `shm_wrapper` and `shm_wrapper_aux`, these two are tools that facilitate the efficient communication between the above three processes. Think of these wrappers as the "glue" that holds the three processes together, and lets them talk to each other.
* **The Logger**: this is the tool that Runtime uses to gather all the logs generated at various places in the code (including by student code) and outputs them to a terminal window, to a file, to Dawn over the network, or to some combination of the three.
* **The Device Code**: codenamed "`lowcar`" <sup id="return2">[2](#footnote2)</sup> this is the set of all the code on the Arduinos that directly control an individual device that is connected to the robot. All of this code is collectively called the "lowcar library".

This README will not go into each of these parts into exhaustive detail; explanations for each part can be found in the corresponding folder in the repo for the interested reader. However, we will describe briefly the data that flows between the various parts, and the manner in which that data is sent:

* `net_handler` communicates with both Dawn and Shepherd. It receives start and stop commands, input data for the coding challenge, and information about the gamepad state. It sends log messages, data about the connected devices, and output data for the coding challenge. This data is sent over both TCP and UDP connections (depending on the type of data) to Shepherd and Dawn, and the data is packaged using Google Protocol Buffers.
* `dev_handler` and `net_handler` are connected via `shm_wrapper`. This connection is used for the `net_handler` to fetch the most recent state of the connected devices, which is then sent to Dawn for the students to see.
* `executor` and `net_handler` are connected via `shm_wrapper_aux` and a FIFO pipe. The connections are used to pass information about the coding challenge (both inputs to the functions and the student's outputs), and for `executor` to know what to run at any given time (autonomous mode, teleop mode, coding challenges, or idle).
* `executor` and `dev_handler` are connected via `shm_wrapper`. This connection is used for `executor` to send commands to the attached devices (tell a motor to run at a certain speed, move a servo to certain position, etc.), and for `dev_handler` to serve `executor` with the device data that it needs to run the student code.
* `dev_handler` communicates with `lowcar` via serial connection. This connection is used to poll for new devices, detect when devices have disconnected, and send device data and commands between the Raspberry Pi and the Arduinos.
* All three processes output log messages through the `logger` tool. If logs are to be sent over the network to Dawn, those logs are put into a FIFO pipe which is opened by `net_handler`, where the logs are processed and sent to Dawn.

## Contents

The following details the folders in the repository:

* `net_handler`: contains routines for managing the TCP and UDP connections to Dawn and Shepherd, the main routine for managing the managers of the connections, the protobuf definitions and compiled protobufs, utility functions specific to `net_handler`, test files, and documentation.
* `dev_handler`: contains routines for managing the connections between the Raspberry Pi and the devices, the main routine for managing the managers of the connections, utilities for unpacking and packing messages to be sent to and from the devices, test files, and documentation.
* `executor`: contains routines for spawning new processes to run the student code (written in Python) in the various modes (autonomous, teleop, coding challenge), an implementation of the StudentAPI (the functions that PiE provides for students to interact with the devices and gamepad) in Cython, the student code itself, test files, and documentation.
* `shm_wrapper` and `shm_wrapper_aux`: contains the implementations of the tools described in the previous section, test files for both wrappers, a simple program in each folder to probe the state of the corresponding shared memory blocks while Runtime is running, and documentation.
* `logger`: contains the implementation of the tool described in the previous section, a configuration file, test files, and documentation.
* `runtime_util`: contains general utility functions that are useful to all of the processes, mostly dealing with getting information about device types and their parameters. The header file defines many important constants that are used throughout Runtime.
* `systemd-stuff`: contains the services that are put into `systemd` that start Runtime on bootup of the Raspberry Pi.

## Dependencies and Concepts

Remember, one of the design principles is KISS. Nevertheless, Runtime is a complex system and will require effort to learn and understand. Hopefully this list does not seem too daunting to you! <sup id="return3">[3](#footnote3)</sup>

Third-party library dependencies:

* `Cython`: this library is used by `executor` to implement the Student API in a way that is both callable from Runtime (which is written in C) and from student code (which is written in Python)
    * Documentation: https://cython.readthedocs.io/en/latest/
* Google `protobuf` and `protobuf-c`: Google `protobuf` is the library that we use to serialize our messages between Runtime and Shepherd, and Runtime and Dawn. The brief explanation of how it works is this: the user defines the structure of a message in "protobuf language", and saves it as a `.proto` file. Google's protobuf compiler will then take that `.proto` file and generate code that can be used in a desired target language to serialize and deserialize ("pack" and "unpack" in the language of protobufs) messages of that type. Since Google's protobuf compiler does not have native support for C, we need to use the third party library `protobuf-c` to generate C code. (But `protobuf-c` makes use of Google's library, so we still need it).
    * `protobuf` documentation: https://developers.google.com/protocol-buffers/docs/proto3
	* `protobuf` Github: https://github.com/protocolbuffers/protobuf
	* `protobuf-c` Github: https://github.com/protobuf-c/protobuf-c

Runtime uses the folllowing C and POSIX standard library headers, in rough order from most commonly used to least commonly used (these all do not require additional installation before they can be used on any POSIX-compliant systems):

* `stdio.h`: for standard input/output (ex. `printf`)
* `stdlib.h`: C standard utility functions (ex. `sleep`, `malloc`, `free`)
* `stdint.h`: C standard integer types (ex. `uint16_t`, `uint32_t`)
* `pthread.h`: for threading functions (ex. `pthread_create`, `pthread_mutex_lock`, `pthread_cond_signal`)
* `unistd.h`: for miscellaneous input/output functions and functions (ex. `read`, `write`, `open`, `close`, `SEEK_END`)
* `errno.h`: for a bunch of error constants and functions (ex. `EEXIST`, `EWOULDBLOCK`, `EINTR`)
* `arpa/inet.h`: for socket functions (ex. `inet_addr`, `bind`, `listen`, `accept`, `socket`)
* `netinet/in.h`: for socket address structures (ex. `struct sockaddr`, `struct sockaddr_in`)
* `sys/types.h`: for some system-defined types for holding various objects (ex. `sem_t` for semaphores)
* `semaphore.h`: for semaphores (ex. `sem_wait`, `sem_post`)
* `sys/mman.h`: for memory mapping used in shared memory (ex. `mmap`)
* `sys/time.h` and `time.h`: for measuring time (ex. `time_t`, `ctime`)
* `Python.h`: for calling Python functions from C used in executor (ex. `Py_XDECREF`, `PyObject_CallObject`)
* `signal.h`: for signal handlers and signal constants (ex. `signal`, `SIGINT`)
* `string.h`: for string manipulation (ex. `strcpy`, `memset`)
* `stdarg.h`: for variable-length arguments used in `logger` (ex. `va_args`)
* `sys/stat.h`: for working with unconventional files (ex. `mkfifo`)
* `sys/wait.h`: for wait functions used in executor to wait for processes (ex. `waitpid`)
* `fcntl.h`: for constants used in opening and closing files (ex. `O_RDONLY`, `O_WRONLY`)

## Running Runtime

TODO!

## Project Organization / Conventions

We use Github Issues to track what we're doing and future initiatives to take up. These issues are loaded into a Github Project and assigned to someone. That person (or people) will create a branch off of whichever branch seems logical to branch off of, and do the work there. 

Try to rebase your changes locally before pushing to Github, so that you don't clog up the public repo's history with small commits. 

Be relatively descriptive in your commit messages (10 - 25 words) and prepend an all-caps tag enclosed in square brackets that describes the part of Runtime you were working on. For example:
```
[NET_HANDLER] logging to the network works but not thoroughly tested, deleted unused constants in net_util, added many comments to net handler files
```
When finished with the feature on your branch, submit a pull request to the branch that you branched off from and request the review of relevant people. Only after someone has reviewed your code and approved the pull request should you then merge (or rebase) your code onto your branch's parent branch (most often `master`).

We do work in two-week cycles. At the beginning of each cycle (or "sprint"), everyone in the project sets goals for what they want to get done during that sprint. At the beginning of each meeting, everyone gives a short update on how they're doing, and what they intend to finish during that meeting. At the end of the cycle, each team member gives a demo of what they finished during that sprint, and the team gives feedback and evaluates the person's work. Of course, this requires the goals to be specific enough to demonstrate during a demo. For example, a goal of "I want to speed up the threads in `dev_handler`" is not specific enough. How will you demonstrate that during a demo? How will you quantify the speedup? How will you measure the speedup? In contrast, a goal of "The executor will be able to successfully terminate student code when Dawn disconnects" is specific enough. You can very clearly demonstrate that this occurs during a demo.

TODO: For coding style conventions, see our style guide.

## Useful Diagrams

TODO!

## Authors

The first version of `c-runtime` was written in the summer of 2020 (when we all had lots of time.... covid sux) nearly entirely by four people (all class of 2022):

* Ben Liao
* Ashwin Vangipuram
* Vincent Le
* Daniel Molina

Some inspiration and design was drawn from previous iterations of Runtime and Lowcar, written mostly by:

* Doug Hutchings
* Yizhe Liu
* Jonathan Lee
* Brose Johnstone
* Sam Zhou
* Brandon Lee

## Footnotes

**<a name="footnote1">1</a>**: Previous iterations of Runtime were written in Python; whether or not this was a good choice is up to the reader's discretion. [↩](#return1)

**<a name="footnote2">2</a>**: This name has an interesting origin. The code for the devices used to be in a completely separate PiE project called "Hibike" (pronounced HEE-bee-kay). Some members of Hibike who wrote much of the original Arduino code were avid fans of anime. So, they named the project Hibike after the anime "Hibike! Yufoniamu" (Sound! Euphonium). Later, some members who were not such avid fans of anime decided to pronounce "Hibike" as "HAI-bike", which sounds like the two English words "High Bike". Later still, when members decided to refactor the code to be more object-oriented, it was decided that a new name was needed for the project. Ben Liao coined the name "Low Car" as wordplay on "High Bike", and the name stuck. [↩](#return2)

**<a name="footnote3">3</a>**: Previous iterations of Runtime, which were written in Python, had some of these third-party dependencies: `zmq`, `protobufs`, `Cython`, as well as several large Python libraries (not all listed): `asyncio`, `threading`, `aio_msgpack_rpc`, `serial_asyncio`, `multiprocessing`, `glob`, `os`, `json` [↩](#return3)