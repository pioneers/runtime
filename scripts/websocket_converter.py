"""Converts the websocket communication from browser Dawn
into a Unix socket communication that Runtime will understand."""


import asyncio
import websockets

WEBSOCKET_PORT = 5000
TCP_PORT = 8101

async def dawn_to_runtime(websocket, writer):
    while True:
        try:
            data = await websocket.recv()
            print("Received data from Dawn:", len(data), "bytes")
        except websockets.exceptions.ConnectionClosed:
            print("dawn_to_runtime: closed connection")
            return
        writer.write(data)
        await writer.drain()
        print("Sent Dawn data to Runtime")

async def runtime_to_dawn(websocket, reader):
    while True:
        data = await reader.read()
        print("Received data from Runtime:", len(data), "bytes")
        try:
            await websocket.send(data)
            print("Sent Runtime data to Dawn")
        except websockets.exceptions.ConnectionClosed:
            print("runtime_to_dawn: closed connection")
            return

async def converter(websocket, path):
    print("Got connection to websocket from:", websocket.remote_address)
    reader, writer = await asyncio.open_connection("127.0.0.1", TCP_PORT)
    await asyncio.gather(dawn_to_runtime(websocket, writer), runtime_to_dawn(websocket, reader), return_exceptions=True)

async def main():
    async with websockets.serve(converter, "127.0.0.1", WEBSOCKET_PORT, ping_interval=None) as server:
        print("Started Websocket Converter")
        await asyncio.Future()  # run forever

if __name__ == '__main__':
    asyncio.run(main())
