CC=gcc
CXX=g++
LIBFLAGS=-pthread

DEV_HANDLER_FILES=message.c ../logger/logger.c ../runtime_util/runtime_util.c arduino-serial-lib.c
CPP_FILES=tests/main.cpp tests/Device.cpp tests/Device.h tests/Messenger.cpp tests/Messenger.h tests/defs.h

# Run dev_handler
dev: dev_handler.c $(DEV_HANDLER_FILES)
	$(CC) $^ -o dev_handler $(LIBFLAGS)
	./dev_handler

# Run dev_handler with Valgrind
val: dev_handler.c $(DEV_HANDLER_FILES)
	$(CC) $^ -o dev_handler $(LIBFLAGS)
	valgrind ./dev_handler

# Run the fake (FILE_DEV) C++ device
cpp: $(CPP_FILES)
	$(CXX) $^ -o file_dev
	./file_dev

# Run the fake (FILE_DEV) C++ device with Valgrind
cpp_val: $(CPP_FILES)
	$(CXX) $^ -o file_dev
	valgrind ./file_dev

# Clean files generated from FILE_DEV
clean:
	rm -f to_dev_handler.txt to_device.txt
