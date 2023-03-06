import asyncio
import time
import uuid
import struct

sHeader = struct.Struct(">B16sI")

#enum NetCommand_t {
COMMAND_PING = 0
COMMAND_PONG = 1
COMMAND_HELLO = 2
COMMAND_SET_SESSIONID = 3
COMMAND_AUTH = 4
COMMAND_OPTIONS = 5
COMMAND_MESSAGE = 6
COMMAND_ERROR = 7
COMMAND_INPUT = 8
COMMAND_FRAMEBUFFER = 9
COMMAND_FLIP = 10
COMMAND_CAMERA = 12
COMMAND_PCM = 13
COMMAND_GOODBYE = 255
#};

class FrameBufferServer:
    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data, addr):
        if len(data) >= 21:
            opcode, session, sequence = sHeader.unpack_from(data)
            message = data[21:]
            print("{}:{} ({}) [{}] {}> ".format(addr[0],addr[1], sequence, uuid.UUID(bytes=session), opcode), message)
            if opcode == COMMAND_PING:
                self.transport.sendto(sHeader.pack(COMMAND_PONG, session, sequence) + message, addr)
                
            elif opcode == COMMAND_HELLO:
                self.transport.sendto(sHeader.pack(COMMAND_HELLO, session, sequence) + bytes([0,0,0,0, 0,0,0,0, 0,0,0,0]), addr)
                print(session)
                if session == b"\0"*16:
                    print("Updating session ID")
                    self.transport.sendto(sHeader.pack(COMMAND_SET_SESSIONID, session, sequence) + uuid.uuid4().bytes, addr)
            
            else:
                print("Unknown opcode {}".format(opcode))
        else:
            print("Received unknown data ", data, "From", addr)


async def main():
    print("Starting UDP server")

    loop = asyncio.get_running_loop()

    transport, protocol = await loop.create_datagram_endpoint(
        lambda: FrameBufferServer(),
        local_addr=('0.0.0.0', 8888))
    
    while True:
        await asyncio.sleep(1)


asyncio.run(main())