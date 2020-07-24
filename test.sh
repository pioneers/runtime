#!/bin/bash

# restore the previous logger config file
function clean_up {
	rm ../../logger/logger.config
	mv ../../logger/logger.config.orig ../../logger/logger.config
}

function run_one_test {
	local found_test=0    # whether or not we found the specified test
	# search through all the tests to find the specified one
	for test in $(ls); do
		# skip Makefile, source files, and anything that doesn't match what was specified
		if [[ $test == "Makefile" || $test == *".c" || $test != *"$1" ]]; then
			continue
		fi
		found_test=1
		break
	done
	
	# exit if we didn't find the test
	if [[ $found_test == 0 ]]; then
		printf "Could not find specified test $1\n"
		clean_up
		exit 1
	fi
	
	# run test
	printf "Running $test...\n"
	./$test
	
	# move the original log file back
	clean_up
	
	# exit with error status of test
	if [[ $? == 1 ]]; then
		exit 1
	fi
	exit 0
}

function run_all_tests {
	local all_is_well=1 # is set to 0 if we have any failed tests
	
	# loop through all tests
	for test in $(ls); do
	    # skip source files and Makefile
	    if [[ $test == "Makefile" || $test == *".c" ]]; then
	        continue
	    fi

		# run test
		printf "Running $test...\n"
		./$test
		
		# if that test failed, set all_is_well to 1
		if [[ $? == 1 ]]; then
			all_is_well=1
		fi
	done

	# move the original log file back
	clean_up

	# return status
	if [[ $all_is_well == 0 ]]; then
		exit 1
	fi
}

################################################ BEGIN SCRIPT ##########################################

# build all of Runtime
./build.sh

# do some setup work with the log file
mv logger/logger.config logger/logger.config.orig
cp -p test/logger/logger.config logger/logger.config

cd test/integration
make

# if first argument specified, try to find that test case and run it
if [[ $1 != "" ]]; then
	run_one_test $1
fi

# if no first argument specified, run all the tests
run_all_tests

exit 0
