#!/bin/bash

udp=9000
tcp=9101
filename='temp_ngrok_url.txt'
url=$(<$filename)
echo "The url used: $url"
rm "$filename"
pipe=/tmp/fifo

trap "rm -f $pipe" EXIT # exit if pipe is removed

if [[ ! -p $pipe ]]; then
    mkfifo $pipe
fi

nc -l -u -p $udp < $pipe | nc $url $tcp > $pipe
