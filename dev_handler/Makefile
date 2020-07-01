CC=gcc
CXX=g++
LIBFLAGS=-lpthread

UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
	#CFLAGS = -I/usr/include/libusb-1.0
endif
ifeq ($(UNAME), Darwin)
	#CFLAGS = -I/usr/local/include/libusb-1.0
	LDFLAGS = -L/usr/local/lib/
endif

# Run dev_handler main
dev: dev_handler.c message.c
	# $(CC) $(CFLAGS) $(LDFLAGS) dev_handler.c message.c ../runtime_util/runtime_util.c -o dev_handler $(LIBFLAGS)
	$(CC) $(CFLAGS) $(LDFLAGS) dev_handler.c message.c ../runtime_util/runtime_util.c tests/arduino-serial-lib.c -o dev_handler $(LIBFLAGS)
	sudo ./dev_handler

# Run dev_handler main with valgrind
val: dev_handler.c
	# $(CC) $(CFLAGS) $(LDFLAGS) dev_handler.c message.c ../runtime_util/runtime_util.c -o dev_handler $(LIBFLAGS)
	$(CC) $(CFLAGS) $(LDFLAGS) dev_handler.c message.c ../runtime_util/runtime_util.c tests/arduino-serial-lib.c -o dev_handler $(LIBFLAGS)
	sudo valgrind ./dev_handler

# Run the dev_handler using serial ports
dev_fake:
	$(CC) tests/fake_handler.c message.c ../runtime_util/runtime_util.c tests/arduino-serial-lib.c -o fake_handler
	sudo ./fake_handler

# Run test for dev_handler
test: dev_test.c
	$(CC) $(CFLAGS) $(LDFLAGS) dev_test.c -o dev_test $(LIBFLAGS)
	sudo ./dev_test

# Run the fake device
fake: tests/fake_device.c tests/fake_device.h
	$(CC) tests/fake_device.c message.c ../runtime_util/runtime_util.c -o fake -lpthread
	sudo ./fake

# Run the fake device with valgrind
fake_val: tests/fake_device.c tests/fake_device.h
	$(CC) tests/fake_device.c message.c ../runtime_util/runtime_util.c -o fake -lpthread
	sudo valgrind ./fake

# Run the fake C++ device
cpp: tests/main.cpp tests/Device.cpp tests/Device.h tests/Messenger.cpp tests/Messenger.h tests/defs.h
	$(CXX) tests/main.cpp tests/Device.cpp tests/Device.h tests/Messenger.cpp tests/Messenger.h tests/defs.h -o fake_cpp
	sudo ./fake_cpp

# Run the fake C++ device on Valgrind
cpp_val: tests/main.cpp tests/Device.cpp tests/Device.h tests/Messenger.cpp tests/Messenger.h tests/defs.h
	$(CXX) tests/main.cpp tests/Device.cpp tests/Device.h tests/Messenger.cpp tests/Messenger.h tests/defs.h -o fake_cpp
	sudo valgrind ./fake_cpp

# Clean files generated from fake
clean:
	rm -f to_dev_handler.txt to_device.txt
