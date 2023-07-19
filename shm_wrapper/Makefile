# list of libraries that shm_wrapper needs to compile
LIBS=-pthread -lrt -Wall

# list of source files that the targets (shm_start and shm_stop) depend on, relative to this folder
SRCS = shm_start.c shm_stop.c shm_wrapper.c ../logger/logger.c ../runtime_util/runtime_util.c

# specify the targets (executables we want to make)
TARGETS = shm_start shm_stop

.PHONY: all clean $(TARGETS)

all: $(TARGETS)

# include top-level Makefile
include ../Makefile_defs

shm_start: $(BIN)/shm_start

shm_stop: $(BIN)/shm_stop

# rule to compile shm_start
$(BIN)/shm_start: $(filter-out %/shm_stop.o, $(OBJS)) | $(BIN)
	$(CC) $^ -o $@ $(LIBS)

# rule to compile shm_stop
$(BIN)/shm_stop: $(filter-out %/shm_start.o, $(OBJS)) | $(BIN)
	$(CC) $^ -o $@ $(LIBS)

# remove build artifacts
clean:
	rm -f $(OBJS)
	rm -f $(patsubst %.o,%.d,$(OBJS))
	rm -f $(BINS)

-include $(patsubst %.o,%.d,$(OBJS))
