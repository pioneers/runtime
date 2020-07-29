#!/usr/bin/env python3

import time

cur = time.time()

duration = 3

print("Start script4.py:", cur, flush=True)
for i in range(duration):
    print(i, flush=True)
    time.sleep(1)
print(duration, flush=True)
