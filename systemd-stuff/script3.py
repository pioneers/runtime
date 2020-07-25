#!/usr/bin/env python3

import time

duration = 7

print("Start script3.py:", time.time(), flush=True)
for i in range(duration):
    print(i, flush=True)
    time.sleep(1)
print(duration, flush=True)
print("End script3.py:", time.time(), flush=True)
