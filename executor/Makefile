cc=gcc
CFLAGS=$(python3.6-config --cflags)
LDFLAGS=$(python3.6-config --ldflags)
LIBS=-lpython3.6m

func: run_python.c
	cc run_python.c -o func $(LIBS)

simple: run_simple_python.c
	cc run_simple_python.c -o simple $(LIBS)

studentapi: studentapi.pyx studentapi.pxd
	python3.6 setup.py build_ext -i

test_api: studentapi
	python3.6 test_studentapi.py

clean:
	rm studentapi.c
	rm studentapi.cpython-36m-x86_64-linux-gnu.so
	rm simple
	rm func