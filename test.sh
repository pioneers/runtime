#!/bin/bash

# restore the previous logger config file
function clean_up {
	rm ../logger/logger.config
	mv ../logger/logger.config.orig ../logger/logger.config
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
		printf "Making $test\n"
		make $test
		
		test_exe=$(echo $test | awk -F'/' '{ print $2 }')
		# run test
		printf "Running $test_exe...\n"
		./$test_exe

		# if that test failed, set all_is_well to 1
		if [[ $? == 1 ]]; then
			failing_tests="$failing_tests $test"  # add this test to list of failing testse
			failed=1
		fi
	done

	clean_up

	# return status
	if [[ $failed == "1" ]]; then
		printf "\n\n################################################# TESTS FAILED! ##########################################\n\n"
		printf "Failing tests: $FAILING_TESTS\n\n\n"
		exit 1
	fi
}

################################################ BEGIN SCRIPT ##########################################

set -e

# build all of Runtime
./build.sh

# do some setup work with the log file
mv logger/logger.config logger/logger.config.orig
cp -p tests/logger.config logger/logger.config

cd tests

export PYTHONPATH="$PYTHONPATH:$PWD/student_code" # modify PYTHONPATH so that executor can find the test student code

make clean

ALL_TESTS=$(ls integration/*.c | awk -F'.' '{ print $1 }')

# if no first argument specified, run all the tests
if [[ $1 == "" ]]; then
	input=$ALL_TESTS
else
	input=$1
fi

printf "\n\n################################################ RUNNING TEST SUITE ##########################################\n\n"

run_tests $input

printf "\n\n############################################### ALL TESTS PASSED ############################################\n\n\n"

exit 0
