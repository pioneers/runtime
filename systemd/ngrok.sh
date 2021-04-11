#!/bin/bash

function sigint_handler {
    pkill -u pi /home/pi/ngrok
}

trap 'sigint_handler' INT

nohup /home/pi/ngrok start --all --config /home/pi/runtime/systemd/ngrok.yml > /dev/null
