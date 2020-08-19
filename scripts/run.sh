#!/usr/bin/env bash

function cleanup() {
    sleep 0.5
    cd shm_wrapper && ./shm_stop
    exit 0
}

cd shm_wrapper && ./shm_start &
trap cleanup INT
sleep 0.5
#cd shm_wrapper && ./static_dev &
#sleep 0.5
cd net_handler && ./net_handler &
sleep 0.5
cd executor && ./executor &
sleep 0.5
cd dev_handler && ./dev_handler &
wait
