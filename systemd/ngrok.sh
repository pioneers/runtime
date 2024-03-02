#!/bin/bash

function sigint_handler {
    pkill -u ubuntu /home/ubuntu/ngrok
}

trap 'sigint_handler' INT

nohup /home/ubuntu/ngrok start --all --config /home/ubuntu/runtime/systemd/ngrok.yml > /dev/null
