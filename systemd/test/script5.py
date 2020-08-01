#!/usr/bin/env python3

import time
import sys

cur = time.time()

duration = 8
exit(1)
print("Start script5.py:", cur, flush=True)
for i in range(duration):
    if i == 5:
        pass
        # sys.exit(1)
    print(i, flush=True)
    time.sleep(1)
print(duration, flush=True)
