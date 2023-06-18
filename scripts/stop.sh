#!/usr/bin/env bash

printf "Killing executor\n"
pkill -INT executor
printf "Killing dev_handler\n"
pkill -INT dev_handler
printf "Killing net_handler\n"
pkill -INT net_handler
printf "Killing network_switch\n"
pkill -INT network_switch
printf "Removing SHM\n"
cd shm_wrapper && ./shm_stop
printf "Killed everything\n"

