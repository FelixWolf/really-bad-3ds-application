import asyncio
import time
import uuid
import struct
from PIL import Image

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
COMMAND_FEATURES = 14
COMMAND_GOODBYE = 255
#};

sUInt32 = struct.Struct(">I")
def packLine(screen, x, y, data, flags = 0):
    header = (len(data) // 3)&511
    header |= (x & 255) << 9
    header |= (y & 511) << 17
    header |= (screen & 3) << 26
    header |= (0 & 15) << 28
    return sUInt32.pack(header) + data

def packFrame(screen, width, height, data, step=5):
    offset = 0
    for y in range(0, height, step):
        result = bytes([0, step])
        for i in range(step):
            result += packLine(screen, 0, y + i, data[offset:offset+(width*3)])
            offset += width * 3
        yield result

def getTestImage():
    im = Image.open("testimg.png").convert("RGB")
    im = im.rotate(-90, expand=True)
    return im.tobytes("raw", "BGR")

image = getTestImage()

import random
sInputs = struct.Struct(">III HH HH HHH HHH f")
class FrameBufferServer:
    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data, addr):
        if len(data) >= 21:
            opcode, session, sequence = sHeader.unpack_from(data)
            message = data[21:]
            if opcode == COMMAND_PING:
                self.transport.sendto(sHeader.pack(COMMAND_PONG, session, sequence) + message, addr)
                
            elif opcode == COMMAND_HELLO:
                self.transport.sendto(sHeader.pack(COMMAND_HELLO, session, sequence) + bytes([0,0,0,0, 0,0,0,0, 0,0,0,0]), addr)
                print(session)
                if session == b"\0"*16:
                    print("Updating session ID")
                    self.transport.sendto(sHeader.pack(COMMAND_SET_SESSIONID, session, sequence) + uuid.uuid4().bytes, addr)
            
            elif opcode == COMMAND_INPUT:
                held, down, up, touchX, touchY, circleX, circleY, \
                accelX, accelY, accelZ, angelX, angleY, angleZ, slider \
                 = sInputs.unpack_from(message)
                print(held, down, up, touchX, touchY, circleX, circleY, \
                accelX, accelY, accelZ, angelX, angleY, angleZ, slider)
                
                for data in packFrame(2, 240, 400, image):
                    result = sHeader.pack(COMMAND_FRAMEBUFFER, session, sequence)
                    self.transport.sendto(result + data, addr)
                    time.sleep(0.0005)
                
                time.sleep(0.001)
                self.transport.sendto(
                    sHeader.pack(COMMAND_FLIP, session, sequence)
                  + bytes([1])
                    , addr)
            else:
                print("{}:{} ({}) [{}] {}> ".format(addr[0],addr[1], sequence, uuid.UUID(bytes=session), opcode), message)
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