#!/usr/bin/env python3

import time
import sys
import os
import zipfile

UPDATE_ZIP = '/tmp/runtime.zip'
DEST = '/home/pi'
DEST = '/tmp'

def update_runtime():
    try:
        with open('/tmp/runtime.zip'):
            pass
    except FileNotFoundError as e:
        return
    with zipfile.ZipFile(UPDATE_ZIP, 'r', compression=zipfile.ZIP_DEFLATED) as zipf:
        print("extracting data")
        zipf.extractall(path=DEST)
    os.remove(UPDATE_ZIP)
    os.system("echo 'Runtime Rebooting' > /tmp/log-fifo ")
    time.sleep(1)
    exit(0)

if __name__ == '__main__':
    while True:
        update_runtime()
        time.sleep(1)
