#!/usr/bin/env python3

import time

end = 5

print("Start script2.py:", time.time())
for i in range(end):
    print(i)
    time.sleep(1)
print(end)
print("End script2.py:", time.time())
