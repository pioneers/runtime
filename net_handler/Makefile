client: client.c
	gcc client.c -o client -lczmq

server: server.c
	gcc server.c -o server -lczmq

test: test_czmq.c
	gcc test_czmq.c -o test -lczmq -lzmq
