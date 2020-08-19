#!/usr/bin/env bash

set -e  # Makes any errors exit the script

pushd shm_wrapper && make && popd
pushd net_handler && make && popd
pushd executor && make && popd
pushd dev_handler && make && popd
