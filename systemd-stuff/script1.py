#!/usr/bin/env python3

import time

end = 10

print("Start script1.py:", time.time())
for i in range(end):
    print(i)
    time.sleep(1)
print(end)
print("End script1.py:", time.time())
