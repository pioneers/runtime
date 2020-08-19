#!/bin/bash

# This file is meant to be sourced by the user, who can then run commands corresponding
# to the name of the script to be run, e.g.
#     runtime build                      (to run build.sh)
#     runtime run                        (to run run.sh)
#     runtime test integration/tc_68_2   (to run the automated test tc_68_2)

# To source this file to make this function available, issue the command
#     source runtime.sh

RUNTIME_DIR=$(dirname "${BASH_SOURCE[0]}")
SCRIPTS=$(ls $RUNTIME_DIR/scripts/ | awk -F '.' '{ print $1 }')

function runtime {
    for script in $SCRIPTS; do
        if [[ $1 == "$script" ]]; then
            shift 1
            printf "Running $RUNTIME_DIR/scripts/$script.sh $@\n\n"
            $RUNTIME_DIR/scripts/$script.sh $@
        fi
    done
}
