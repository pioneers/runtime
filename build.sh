#!/usr/bin/env bash

set -e

pushd executor && make && popd
pushd net_handler && make && popd
pushd shm_wrapper && make && popd
pushd dev_handler && make && popd
