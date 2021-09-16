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
	exit 1
}

function run_tests {
	local test_exe=""     # name of executable for test
	local failed=0		  # whether tests failed
	local failing_tests=""

	echo "Running tests: $@"

	for test in $@; do
		# exit if we didn't find the test
		if [[ $ALL_TESTS != *"$test"* ]]; then
			printf "Could not find specified test $test\n"
			clean_up
			exit 1
		fi

		# make executable
		printf "\nMaking $test\n"
		make $test

		# if test failed to compile, set failed to 1
		if [[ $? != 0 ]]; then
			failing_tests="$failing_tests $test"  # add this test to list of failing tests
			failed=1
            continue
		fi

		# run test
		printf "Running $test...\n"
		./$test

		# if that test failed, set failed to 1
		if [[ $? != 0 ]]; then
			failing_tests="$failing_tests $test"  # add this test to list of failing tests
			failed=1
		fi
	done

	clean_up

	# return status
	if [[ $failed == "1" ]]; then
		printf "\n\n################################################# TESTS FAILED! ##########################################\n\n"
		printf "Failing tests: $failing_tests\n\n\n"
		exit 1
	fi
}

################################################ BEGIN SCRIPT ##########################################

# installs function clean_up as SIGINT handler
trap 'sigint_handler' INT

# build all of Runtime
./runtime build

# replace the logger config with the production config
mv logger/logger.config logger/logger.config.orig
cp -p tests/logger.config logger/logger.config

cd tests

make devices

INT_TESTS=$(ls integration/*.c | awk -F'.' '{ print $1 }')
PERF_TESTS=$(ls performance/*.c | awk -F'.' '{ print $1 }')
ALL_TESTS="$INT_TESTS $PERF_TESTS"

# if no first argument specified, run all the tests
if [[ $1 == "" || $1 == "all" ]]; then
	input=$ALL_TESTS
elif [[ $1 == "int" ]]; then
    input=$INT_TESTS
elif [[ $2 == "perf" ]]; then
    input=$PERF_TESTS;
else
	input=$1
fi

printf "\n\n################################################ RUNNING TEST SUITE ##########################################\n\n"

run_tests $input

printf "\n\n############################################### ALL TESTS PASSED ############################################\n\n\n"

exit 0
