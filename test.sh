#!/bin/bash

# build all of Runtime, then compile all the tests
./build.sh
cd test/integration
make

# run all the tests
for test in $(ls); do
    # skip source files and Makefile
    if [[ $test == "Makefile" || $test == *".c"  ]]; then
        continue
    fi

	./$test
	# if that test failed, exit 1
	if [[ $? == 1 ]]; then
		exit 1
	fi
done

exit 0
