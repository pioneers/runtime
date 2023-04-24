#!/bin/bash

# This script runs all of Runtime in a terminal

# function to clean up shared memory
function cleanup() {
    sleep 0.5
    cd bin && ./shm_stop
    exit 0
}

cd bin && ./shm_start &
trap cleanup INT # sets cleanup as Ctrl-C handler
sleep 0.5

# starts all three Runtime processes in the background
cd bin && ./net_handler &
sleep 0.5
cd bin && ./executor &
sleep 0.5
cd bin && ./dev_handler &
sleep 0.5
cd bin && ./network_switch & 

# waits for background processes to stop (they won't ever stop)
wait
