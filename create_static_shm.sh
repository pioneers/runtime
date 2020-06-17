#!/usr/bin/env bash

shm_wrapper/static &
pid[0] = $!
shm_wrapper_aux/static &
pid[1] = $!
trap INT " kill -SIGINT ${pid[0]} ${pid[1]}"
wait
