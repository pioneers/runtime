import asyncio
import websockets
import socket

async def reverse_converter():
    server = socket.socket()
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("localhost", 6000))
    server.listen()
    conn, address = server.accept()
    print("Received connection from:", address)

    async with websockets.connect("ws://localhost:5000") as websocket:
        while True:
            data = conn.recv(4096)
            print("Received data from net_handler_client", len(data), "bytes")
            await websocket.send(data)
            print("Sent client data to Net Handler websocket")

if __name__ == '__main__':
    asyncio.run(reverse_converter())
