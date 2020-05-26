cc = gcc
CFLAGS = $(python3.6-config --cflags)
LDFLAGS = $(python3.6-config --ldflags)

func: run_python.c
	cc $(CFLAGS) run_python.c $(LDFLAGS) -o func.out

simple: run_simple_python.c
	cc $(CFLAGS) run_simple_python.c $(LDFLAGS) -o simple.out

studentapi: studentapi.pyx studentapi.pxd
	python3.6 setup.py build_ext -i