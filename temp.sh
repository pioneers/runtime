#!/bin/bash

pkill net_handler
pkill dev_handler
rm -f /dev/shm/*
rm -f /tmp/ttyACM*
