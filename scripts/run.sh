#!/bin/bash

# This script runs all of Runtime in a terminal

# function to clean up shared memory
function cleanup() {
    sleep 0.5
    cd shm_wrapper && ./shm_stop
    exit 0
}

cd shm_wrapper && ./shm_start &
trap cleanup INT # sets cleanup as Ctrl-C handler
sleep 0.5

# starts all three Runtime processes in the background
cd net_handler && ./net_handler &
sleep 0.5
cd executor && ./executor &
sleep 0.5
cd dev_handler && ./dev_handler &

# waits for background processes to stop (they won't ever stop)
wait
