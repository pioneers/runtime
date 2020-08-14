#!/bin/bash

# normal clean up function that restores logger config
function clean_up {
	rm ../logger/logger.config
	mv ../logger/logger.config.orig ../logger/logger.config
}

# sigint handler
function sigint_handler {
	# if logger/logger.config.orig exists, we need to restore it
	if [[ -f ../logger/logger.config.orig ]]; then
        clean_up
	fi
	# if temp.txt exists, we Ctrl-C'ed in the middle of a running test and we need to remove it
	if [[ -f /tmp/temp_logs.txt ]]; then
		rm /tmp/temp_logs.txt
	fi
	exit 1
}

function run_tests {
	local test_exe=""     # name of executable for test
	local failed=0		  # whether tests failed
	local failing_tests=""

	printf "Running tests: $@ \n"

	for test in $@; do
		# exit if we didn't find the test
		if [[ $ALL_TESTS != *"$test"* ]]; then
			printf "Could not find specified test $test\n"
			clean_up
			exit 1
		fi

		# run test
		printf "\nRunning $test...\n"
		test_exe=$(echo $test | cut -c 7-) # Get rid of 'tests/' from file name
		python3 $test_exe
		local exit_code=$?
		printf "\n"

		# if that test failed, add to list
		if [[ $exit_code == 1 ]]; then
			failing_tests="$failing_tests $test"  # add this test to list of failing testse
			failed=1
		fi
	done

	clean_up

	# return status
	if [[ $failed == 1 ]]; then
		printf "\n################################################# TESTS FAILED! ##########################################\n"
		printf "Failing tests: $failing_tests\n\n"
		exit 1
	fi
}

################################################ BEGIN SCRIPT ##########################################

# installs function clean_up as SIGINT handler
trap sigint_handler INT

# build all of Runtime
make build

# do some setup work with the log file
mv logger/logger.config logger/logger.config.orig
cp -p tests/logger.config logger/logger.config

ALL_TESTS=$(ls tests/integration/*.py)

cd tests
make tester.c

# if no first argument specified, run all the tests
if [[ $1 == "" ]]; then
	input=$ALL_TESTS
else
	input=$1
fi

printf "\n\n############################################ RUNNING TEST SUITE ##########################################\n\n"

run_tests $input

printf "\n############################################### ALL TESTS PASSED ############################################\n\n"

exit 0
