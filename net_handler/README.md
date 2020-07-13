# Network Handler

The network handler's job is to communicate with Dawn and Shepherd through sockets and turn the network messages into shared memory updates.

## Details

The communication streams can be split into a TCP connection and UDP connection. Both Dawn and Shepherd will a TCP connection and only Dawn will have a UDP connection. These connections are commonly known as *sockets*. A socket can be thought of as a messenger that can send and receive messages from other sockets over various interfaces like Ethernet, Wifi, or even on different processe in same machine.

### Structure
* protos/ - contains the Protobuf definitions of all our network messages
* pbc_gen/ - the corresponding C code that is generated from the Protobufs using protobuf-c
* protobuf_tests/ - contains several test files to ensure that the Protobuf message passing/reading works
* test/ - contains test clients for TCP and UDP that imitate Dawn and Shepherd
* net_handler.c - main entry file that starts UDP and accepts TCP connections
* net_util.c - helper functions to communicate with the TCP sockets
* tcp_conn.c - handles the TCP connection
* udp_conn.c - handles the UDP connection

### Protobufs

To use sockets effectively, both ends need to use a unified messaging protocol. We use the well known Google Protobufs https://developers.google.com/protocol-buffers/docs/proto3 to convert our data into byte streams. The idea is to have one message framework used by all PiE services. Then in each language that uses the socket, have an API to pack/unpack the messages. Since Google doesn't provide an API for C, we use the third-party `protobuc-c` library https://github.com/protobuf-c/protobuf-c. Examples on how to use the library can be found in its wiki. The message definitions Runtime uses are defined in `protos/`. Each message type received will correspond to a message definition `.proto` file.  To see all the message types and which client use them, see [here](#message-table). 

### TCP

TCP is a stream-based protocol, so there is no built in way to determine when a message ends. As a result, we designed a custom messaging scheme where the first byte we read will determine the message type. The next two bytes are the length of the Protobuf message. Then we read that number of bytes from the stream. We can then convert those bytes into the designated message type using `protobuf-c`. This message reading is done in a loop inside `recv_new_message` in `tcp_conn.c`. From there we can take the corresponding action.  Each message type has a corresponding action. RUN_MODE and START_POS simply update auxiliary shared memory.

### UDP

UDP is a packet-based protocol, and so we can just send/receive a packet at a time. Since the receive and send streams only contain one message type, we know that the packets we receive are only `GpState` and the packets we send are only `DevData`.  However, UDP may sometimes drop packets during the transmission. As a result, we have 2 threads: 1 to receive the gamepad data and 1 to send the device data. To know where to send the data, we first wait to receive gamepad data from the client. Then when both threads run, `recvfrom` will update the UDP client address if it ever changes. 

### Message Table

| TCP Receive    | Dawn | Shepherd |
|----------------|------|----------|
| RUN_MODE       | X    | X        |
| START_POS      | X    | X        |
| CHALLENGE_DATA | X    | X        |
| DEVICE_DATA    | X    |          |

| TCP Send       | Dawn | Shepherd |
|----------------|------|----------|
| CHALLENGE_DATA | X    | X        |
| LOG            | X    |          |

| UDP Receive    | Dawn | Shepherd |
|----------------|------|----------|
| GP_STATE       | X    | N/A      |

| UDP Send       | Dawn | Shepherd |
|----------------|------|----------|
| DEVICE_DATA    | X    | N/A      |

### Actions:
* RUN_MODE: Update the run mode in the auxiliary shared memory
* START_POS: Update the start position in the auxiliary shared memory
* CHALLENGE_DATA (receive): Send the challenge inputs to the executor and then start running coding challenges
* CHALLENGE_DATA (send): Send the challenge outputs from the executor
* DEVICE_DATA (receive): Set what device data the UDP thread sends to the client
* DEVICE_DATA (send): Send the up-to-date device data from shared memory
* LOG: Send any logging statements
* GP_STATE: Update the gamepade state in auxiliary shared memory

### Challenges

The challenge inputs and outputs are communicated with the executor using UNIX sockets. These sockets don't use the IP protocol like the other sockets, but instead some UNIX protocol. This is often called an interprocess communication (IPC) since it is a tool to talk between processes. The `net_handler` will send any challenge inputs it receives to the socket and then begins the `CHALLENGE` mode. Once the executor finished running the challenges, it will return the results as C strings in the socket. The `net_handler` will then read this and package this into a `Text` protobuf to send to the TCP client. An important thing to note is that starting the challenges and receiving its results are asynchronous and the `net_handler` will send the results whenever they are ready. 

## Building

You can make all files with `make`. If you want to make them individually, first make the protobuf definitions with `make gen_proto`. Then make the `net_handler` with `make net_handler`. Finally if you want to make the test clients, do `make tcp_client` or `make udp_client`.

## Testing

The device handler must be running before the network handler starts. You can do this using a dummy `dev_handler` by doing `cd ../shm_wrapper && make static` then `./static`. Then you can spawn the `net_handler` and `executor` with `./net_handler` in one terminal and `./executor` in another. Make sure that you've built the `executor` first, instructions can be found in the `executor` README. Finally you can test either TCP or UDP by running `./tcp_client` or `./udp_client` respectively.

