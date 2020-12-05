#!/usr/bin/env bash

## To package a new runtime.zip from source code, run
## zip -r runtime.zip runtime
## zip -r runtime.zip -d "runtime/.git/*"

set -e

UPDATE_ZIP='/tmp/runtime.zip'
DEST='/home/pi/'

while [ ! -f $UPDATE_ZIP ]
do
  sleep 2
done

echo 'IMPORTANT: Runtime Rebooting for Update' >> /tmp/log-fifo
sleep 2

SERVICES="executor dev_handler net_handler shm_stop shm_start"

cd systemd && sudo systemctl stop $SERVICES

unzip -o $UPDATE_ZIP -d $DEST

printf "Zip file extracted, robot rebooting\n"
sudo shutdown -r now
