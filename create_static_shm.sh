#!/usr/bin/env bash

shm_wrapper/static &
p2=$!
shm_wrapper_aux/static
p2=$!
#trap INT " kill -s INT ${p1} ${p2}"
wait
