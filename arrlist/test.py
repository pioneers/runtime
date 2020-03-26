import time

def test_loop():
     lst = []
     for i in range(1000000):
             lst.append(i)
        
start = time.time()

test_loop()

end = time.time()

print(end - start)