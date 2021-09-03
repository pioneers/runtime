# Network Handler

The network handler's job is to communicate with Dawn and Shepherd through sockets and turn the network messages into shared memory updates. Learn more about how the network handler works in our [wiki](https://github.com/pioneers/c-runtime/wiki/Network-Handler). For tutorials and resources regarding Google protobufs, check out the [Protobuf wiki page](https://github.com/pioneers/runtime/wiki/Protobufs).

### Structure
* `protos/` - contains the Protobuf definitions of all our network messages. This is a [submodule](https://github.com/pioneers/protos).
* `pbc_gen/` - the corresponding C code that is generated from the Protobufs using protobuf-c
* `net_handler.c` - main entry file that accepts TCP connections from Dawn/Shepherd
* `net_util.c` - helper functions to communicate with the TCP sockets
* `tcp_conn.c` - handles the TCP connection with a specific client

## Building

You can make all files with `make`. If you want to make them individually, first make the protobuf definitions with `make gen_proto`. Then make the `net_handler` with `make net_handler`. 

## Testing

The `net_handler` can be tested using the `net_handler_cli` in the `tests` directory; many automated tests in `tests/integration` also work to test the `net_handler`'s functionality. Shared memory must exist before `net_handler` can be run successfully. See the `README` in the `tests` folder, as well as the [testing framework wiki page](https://github.com/pioneers/runtime/wiki/Test-Framework) for more information.
