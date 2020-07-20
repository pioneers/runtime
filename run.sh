#/usr/bin/env bash

cd shm_wrapper && ./shm &
sleep 1
cd shm_wrapper && ./static_dev &
sleep 1
cd net_handler && ./net_handler &
sleep 1
cd executor && ./executor &
wait