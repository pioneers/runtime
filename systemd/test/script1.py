#!/usr/bin/env python3

import time
import sys

cur = time.time()

duration = 10

print("Start script1.py:", cur, flush=True)
for i in range(duration):
    if i == 7:
        sys.exit(1)
    print(i, flush=True)
    time.sleep(1)
print(duration, flush=True)
