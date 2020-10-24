#!/usr/bin/env python3

import time
import sys
import os
import zipfile

UPDATE_ZIP = '/tmp/runtime.zip'
DEST = '/home/pi/test'

def update_runtime():
    try:
        with open('/tmp/runtime.zip'):
            pass
    except FileNotFoundError as e:
        return
    os.system("echo 'IMPORTANT: Runtime Rebooting for Update' > /tmp/log-fifo ")
    time.sleep(2)
    os.system("cd systemd && sudo systemctl stop *.service")
    with zipfile.ZipFile(UPDATE_ZIP, 'r') as zipf:
        zipf.extractall(path=DEST)
    print("Zip file extracted, robot rebooting")
    # os.system("sudo shutdown -r now")
    exit(0)

if __name__ == '__main__':
    while True:
        update_runtime()
        time.sleep(1)
