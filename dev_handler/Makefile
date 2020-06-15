CC=gcc
LIBFLAGS=-lusb-1.0 -lpthread
UNAME_S := $(shell uname -s) # Get Unix name

# Mac
ifeq ($(UNAME_S), Darwin)
	CFLAGS = -I/usr/local/include/libusb-1.0
	LDFLAGS = -L/usr/local/lib/
endif

# Linux
ifeq ($(UNAME_S), Linux)
	CFLAGS = -I/usr/include/libusb-1.0
endif

# Run dev_handler main
dev: dev_handler.c message.c
	$(CC) $(CFLAGS) $(LDFLAGS) dev_handler.c message.c ../runtime_util/runtime_util.c -o dev_handler $(LIBFLAGS)
	sudo ./dev_handler

# Run dev_handler main with valgrind
val: dev_handler.c
	$(CC) $(CFLAGS) $(LDFLAGS) dev_handler.c message.c ../runtime_util/runtime_util.c -o dev_handler $(LIBFLAGS)
	sudo valgrind ./dev_handler

# Run test for dev_handler
test: dev_test.c
	$(CC) $(CFLAGS) $(LDFLAGS) dev_test.c -o dev_test $(LIBFLAGS)
	sudo ./dev_test
