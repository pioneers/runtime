#/usr/bin/env bash

cd shm_wrapper && ./shm &
sleep 0.5
cd shm_wrapper && ./static_dev &
sleep 0.5
cd net_handler && ./net_handler &
sleep 0.5
cd executor && ./executor &
sleep 0.5
cd dev_handler && ./dev_handler &
wait
