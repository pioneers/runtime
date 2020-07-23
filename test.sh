#!/bin/bash

INTEGRATION_TESTS=$(ls test/integration | awk '{ 
		for (i=1; i <= NF; i++) {
			tmp=match($i, /^Makefile$/)
			if (!tmp) {
				print $i
			}
		}
	}'
)
	
# compile all the tests
cd test/integration && make

# run all the tests
for test in INTEGRATION_TESTS; do
	cd /test/integration && ./$test
	# if that test failed, exit 1
	if [[ $? == 1 ]]; then
		exit 1
	fi
done

exit 0
