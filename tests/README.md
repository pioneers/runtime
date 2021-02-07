# Test Framework

This folder contains all the files that we use for manual testing, interacting with Runtime without Dawn, and writing automated tests. The script `scripts/test.sh` is the file that actually runs the automated tests, and is also part of the test framework. To run the script, run the command `runtime test <test number>` (see the last section of this README). For a more thorough discussion of testing philosophy and how these test clients work, see the [corresponding page in the Wiki](https://github.com/pioneers/runtime/wiki/Test-Framework).


## Contents

The following is an overview of the structure of this folder:

* `client/*`: this folder contains the headers and the implementations of all of the clients that we have for accessing the different parts of Runtime. There is a client for `net_handler`, another for `executor`, another for `dev_handler`, and a fourth for `shm`. These clients are used by both the Command Line Interfaces (CLIs) and the automated tests to issue commands to Runtime.
* `cli/*`: this folder contains all of the command line interfaces that we can use to interactively issue commands to Runtime from the command line. There is a CLI for each client mentioned above.
* `integration/*`: this folder contains all of our integration tests. Each test is compiled and run automatically by our continuous integration (CI) tool, Travis, to verify whenever someone submits a pull request that the code works and doesn't break previous behavior. The naming scheme for the tests is explained briefly in the Description section of this README, but is explained in more detail on the wiki.
* `student_code/*`: this folder contains all of the test student code that the integration tests use to test a certain condition. For example, suppose you were writing a test that aims to show that `executor` raises an error when student code times out. You would need to write student code that times out, put it in this folder, then write an integration test in `integration/` which runs `executor` on the student code you just wrote, and verify that that behavior is indeed seen. This folder is also searched when running the CLI, so we can run the CLI with any code in this folder too, without needing to touch `executor/studentcode.py`.
* `Makefile`: this file is used to make the CLI executables, as well as the executables for all the integration tests
* `logger.config`: this file defines the logger configuration that we use to run tests (since the options that we have for testing are different from the options that we have for production).
* `test.c and test.h`: these files define the interface that the automated tests use to run tests, in addition to the interface presented to the tests by the clients. The functions defined here all have to do with starting and setting up the tests, and comparing the output of a test with its expected output. See the wiki for more on how this works.

## Running the CLI

First, do `make cli` in this directory. This will create four executables: `net_handler_cli`, `executor_cli`, `dev_handler_cli`, and `shm_ui`. Open up four terminal windows and navigate to this directory in all four terminal windows. Then, do:

1. `./shm_ui` in one of the terminal windows. This will create the shared memory and get Runtime ready to run on top of it.
	- If Runtime is already running (for example, with `systemd`), and you want to simply view the state of existing shared memory blocks, run with `./shm_ui attach`
2. `./net_handler_cli`, `./executor_cli`, and `./dev_handler_cli` in the other three terminals, which start up `net_handler`, `executor`, and `dev_handler` in "test mode", respectively (i.e. `net_handler` doesn't try to make data available on a publicly viewable port, and `dev_handler` doesn't try to look for actual Arduino devices; it instead looks for "fake devices" that are spawned by `dev_handler_client`).
3. In the `executor_cli` window, specify which student code you want to run. You can specify `studentcode` to run the actual student code in `executor/studentcode.py`, or you can specify any of the files under the `student_code` folder in this folder.
4. Type `help` into each of the four windows to get a list of available commands. You're now ready to experiment with Runtime! Send it inputs from the network via `net_handler_cli`; simulate connecting / disconnecting devices with `dev_handler_cli`, and view the state of shared memory in real time with `shm_ui`.

## Running Automated Tests

Automated tests should never be run from this directory; just use the shell script in the top-level directory. The test script has some built-in cleanup functions if you press `Ctrl-C` in the middle of running a test, and helpful error messages and status messages that will be helpful in understanding what's going on. It also builds Runtime for you before running the test, so that you can make sure you're running the test with the latest code changes you may have made locally on your machine.

To run all tests, simply run `runtime test` from any directory. This will take a while to run, so you usually don't need to run this unless you're about to push some major updates to the code to Github and want to verify that the tests pass before you push.

To run one or more tests, run `runtime test integration/tc_<num> <... list all tests you want to run ... >` from the top-level directory.