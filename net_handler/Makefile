CFLAGS=$(shell pkg-config --cflags libprotobuf-c)
LDFLAGS=$(shell pkg-config --libs libprotobuf-c)
LIBS=-pthread -lrt -Wall

GEN_FOLDER=pbc_gen
GEN_FILES:=$(GEN_FOLDER)/*.c

all: $(GEN_FOLDER)/* net_handler

# makes the generated protobuf-c files in the pbc_gen folder
$(GEN_FOLDER)/%: $(wildcard protos/%.proto)
	cd protos && protoc-c --c_out ../$(GEN_FOLDER) $*.proto

clean:
	rm -f $(GEN_FOLDER)/*
	rm -f net_handler

# makes the actual net_handler executable
net_handler: net_util.c net_handler.c udp_conn.c tcp_conn.c ../logger/logger.c ../shm_wrapper/shm_wrapper.c ../runtime_util/runtime_util.c 
	gcc $^ $(GEN_FILES) -o $@ $(LIBS) $(LDFLAGS)
