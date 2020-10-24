#!/usr/bin/env bash

set -e

UPDATE_ZIP='/tmp/runtime.zip'
DEST='/home/pi/test'

while [ ! -f $UPDATE_ZIP ]
do
  sleep 2
done

echo 'IMPORTANT: Runtime Rebooting for Update' >> /tmp/log-fifo
sleep 2

cd systemd && sudo systemctl stop *.service

unzip -o /tmp/runtime.zip -d ~/

printf "Zip file extracted, robot rebooting\n"


# sudo shutdown -r now
