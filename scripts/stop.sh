#!/usr/bin/env bash

printf "first here\n"
pkill executor
printf "second\n"
pkill dev_handler
printf "third\n"
pkill net_handler
printf "fourth\n"
cd shm_wrapper && ./shm_stop
printf "Killed everything\n"
