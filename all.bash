#!/bin/bash

# This file is meant to be sourced by the user, who can then run commands corresponding
# to the name of the script to be run, e.g.
#     runtime build                      (to run build.sh)
#     runtime run                        (to run run.sh)
#     runtime test integration/tc_68_2   (to run the automated test tc_68_2)
# To source this file, issue the command
#     source all.bash

SCRIPTS="run build test"

function runtime {
    for script in $SCRIPTS; do
        if [[ $1 == "$script" ]]; then
            shift 1
            printf "Running ./scripts/$script.sh $@\n\n"
            ./scripts/$script.sh $@
        fi
    done
}