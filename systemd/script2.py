#!/usr/bin/env python3

import time

duration = 5

print("Start script2.py:", time.time(), flush=True)
for i in range(duration):
    print(i, flush=True)
    time.sleep(1)
print(duration, flush=True)
print("End script2.py:", time.time(), flush=True)
