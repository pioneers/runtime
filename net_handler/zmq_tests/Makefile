CZMQ=$(shell pkg-config --libs --cflags libczmq)
ZMQ= $(shell pkg-config --libs --cflags libzmq)


client: client.c
	gcc client.c -o client -lczmq

server: server.c
	gcc server.c -o server -lczmq

test: test_czmq.c
	gcc test_czmq.c -o test $(CZMQ) $(ZMQ)

