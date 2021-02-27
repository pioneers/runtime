#!/bin/bash

function sigint_handler {
    pkill -u pi /home/pi/ngrok
    pkill -u pi ngrok_displayer
}

trap 'sigint_handler' INT

nohup /home/pi/ngrok start --all --config /home/pi/runtime/systemd/ngrok.yml > /dev/null &

sleep 5

python3 ngrok_displayer.py > /dev/null

