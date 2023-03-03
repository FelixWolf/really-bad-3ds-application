import asyncio
import time
class FrameBufferServer:
    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data, addr):
        print("Received", data, "From", addr)
        self.transport.sendto(b"Hello!", addr)


async def main():
    print("Starting UDP server")

    loop = asyncio.get_running_loop()

    transport, protocol = await loop.create_datagram_endpoint(
        lambda: FrameBufferServer(),
        local_addr=('0.0.0.0', 8888))
    
    while True:
        await asyncio.sleep(1)


asyncio.run(main())