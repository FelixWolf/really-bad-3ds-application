import asyncio
import socket
import struct
from PIL import Image
import random

u32 = struct.Struct(">I")

class FrameBufferClient(asyncio.Protocol):
    def __init__(self, reader, writer):
        self.reader = reader
        self.writer = writer
        self.transport = writer.transport
    
    async def run(self):
        await self.connectionMade()
        while not self.reader.at_eof() and not self.writer.is_closing():
            data = await self.reader.read(1)
            if not data:
                break
            if data == b"\x08":
                print(await self.reader.read(36))
            elif data == b"\x03":
                print ("Frame request")
                im = Image.new("RGB", (400,240), random.randint(0,0xFFFFFF))
                data = im.tobytes('raw', 'BGR')
                await self.write(b"\x04\x00" + u32.pack(len(data)) + data)
                
                im = Image.new("RGB", (400,240), random.randint(0,0xFFFFFF))
                data = im.tobytes('raw', 'BGR')
                await self.write(b"\x04\x01" + u32.pack(len(data)) + data)
                
                im = Image.new("RGB", (320,240), random.randint(0,0xFFFFFF))
                data = im.tobytes('raw', 'BGR')
                await self.write(b"\x04\x02" + u32.pack(len(data)) + data)
            else:
                print(data)
        await self.connectionLost()
    
    async def write(self, data):
        if not self.writer.is_closing():
            self.writer.write(data)
            await self.writer.drain()
    
    async def close(self):
        if not self.writer.is_closing():
            self.writer.close()
            await self.writer.wait_closed()
            return True
        return False
    
    async def connectionMade(self):
        peername = self.transport.get_extra_info('peername')
        print('Connection from {}'.format(peername))
    
    async def connectionLost(self):
        peername = self.transport.get_extra_info('peername')
        print("{} disconnected".format(peername))

async def acceptClient(reader, writer):
    fbc = FrameBufferClient(reader, writer)
    await fbc.run()

async def main():
    server = await asyncio.start_server(acceptClient, '0.0.0.0', 8888)

    addrs = ', '.join(str(sock.getsockname()) for sock in server.sockets)
    print(f'Serving on {addrs}')

    async with server:
        await server.serve_forever()

asyncio.run(main())