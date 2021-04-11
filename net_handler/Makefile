CFLAGS=$(shell pkg-config --cflags libprotobuf-c)
LDFLAGS=$(shell pkg-config --libs libprotobuf-c)
LIBS=-pthread -lrt -Wall

GEN_FOLDER=pbc_gen
GEN_FILES:=$(GEN_FOLDER)/*.c

all: $(GEN_FOLDER)/* net_handler converter

# makes the generated protobuf-c files in the pbc_gen folder
$(GEN_FOLDER)/%: $(wildcard protos/%.proto) 
	mkdir -p $(GEN_FOLDER)
	cd protos && protoc-c --c_out ../$(GEN_FOLDER) $*.proto

clean:
	rm -rf $(GEN_FOLDER)
	rm -f net_handler

# makes the actual net_handler executable
net_handler: net_util.c net_handler.c udp_conn.c tcp_conn.c ../logger/logger.c ../shm_wrapper/shm_wrapper.c ../runtime_util/runtime_util.c 
	gcc $^ $(GEN_FILES) -o $@ $(LIBS) $(LDFLAGS)

############################## UDP_TCP_CONVERTER ##############################
# This is specifically for 2021 Spring Comp!
converter: udp_tcp_converter udp_tcp_converter_test

udp_tcp_converter: udp_tcp_converter.c net_util.c ../logger/logger.c ../runtime_util/runtime_util.c
	gcc $^ $(GEN_FILES) -o $@ $(LDFLAGS)

udp_tcp_converter_test: udp_tcp_converter_test.c net_util.c ../logger/logger.c ../runtime_util/runtime_util.c
	gcc $^ $(GEN_FILES) -o $@ $(LDFLAGS)
