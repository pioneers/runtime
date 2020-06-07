CC=gcc
LIBFLAGS=-lusb-1.0
UNAME := $(shell uname) # Get Unix name

ifeq ($(UNAME), Darwin)
	# Mac
	CFLAGS +=-I/usr/local/include/libusb-1.0
	LDFLAGS +=-L/usr/local/lib/
else
	# Linux
	CFLAGS += -I/usr/include/libusb-1.0
endif

dev: dev_handler.c
	$(CC) $(CFLAGS) $(LDFLAGS) dev_handler.c -o dev_handler $(LIBFLAGS)
	sudo ./dev_handler

val: dev_handler.c
	$(CC) $(CFLAGS) $(LDFLAGS) dev_handler.c -o dev_handler $(LIBFLAGS)
	sudo valgrind ./dev_handler
