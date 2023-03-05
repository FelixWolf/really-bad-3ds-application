#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <malloc.h>
#include <cstring>

#include "common.h"
#include "network.h"
#include "input.h"
#include "main.h"

#include <3ds.h>

enum NFBState_t {
    DISCONNECTED,
    AUTHENTICATING,
    CONNECTED
};

void sendMessage(Network *net, uint32_t sequence, uint8_t command, uint8_t session[16], uint8_t *data, int datalen){
    uint8_t result[17 + datalen];
    result[0] = command;
    memcpy(result+1, session, 16);
    memcpy(result+17, data, datalen);
    net->send(result, sizeof(result));
}

enum NetCommand_t {
    COMMAND_PING = 0,
    COMMAND_PONG = 1,
    COMMAND_HELLO = 2,
    COMMAND_SET_SESSIONID = 3,
    COMMAND_AUTH = 4,
    COMMAND_OPTIONS = 5,
    COMMAND_MESSAGE = 6,
    COMMAND_ERROR = 7,
    COMMAND_INPUT = 8,
    COMMAND_FRAMEBUFFER = 9,
    COMMAND_FLIP = 10,
    COMMAND_CAMERA = 12,
    COMMAND_PCM = 13,
    COMMAND_GOODBYE = 255
};

int main(int argc, char **argv)
{
    // Initialize services
    gfxInitDefault();
    
    //Enable HIDs
    HIDUSER_EnableAccelerometer();
    HIDUSER_EnableGyroscope();
    
    //Initialize networking
    netInit();
    
    //Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
    consoleInit(GFX_TOP, NULL);
    
    Network net;
    
    InputMap_t inputPrevious;
    InputMap_t inputCurrent;
    
    NFBState_t state;
    uint32_t send_sequence = 0;
    uint8_t SessionID[16];
    // Main loop
    while (aptMainLoop())
    {
        getInputs(&inputCurrent);
        
        if(net.connected()){
            if(state == CONNECTED && shouldSendControls(&inputCurrent, &inputPrevious)){
                uint8_t *arr[CONTROL_PACK_SIZE];
                packControls(&inputCurrent, arr);
                //Send back the ping as COMMAND_PONG with the received data
                sendMessage(&net, send_sequence++, COMMAND_INPUT, SessionID, (uint8_t*)arr, sizeof(arr));
                inputPrevious = inputCurrent;
            }
            
            //This escapes when nothing left to read, or max processing time reached(TODO)
            while(true){
                uint8_t buffer[0xFFFF];
                int received = net.recv(buffer, sizeof(buffer));
                if(received <= 0)
                    break; //No data? No more messages
                
                if(memcmp(buffer+4, SessionID, 16) != 0)
                    continue; //Not for us
                
                uint32_t receive_sequence;
                
                //Skip command(1), session ID(16), and sequence(4)
                uint8_t *data = buffer + 21;
                received -= 21;
                
                switch(buffer[0])
                {
                    case COMMAND_PING:
                    {
                        //Send back the ping as COMMAND_PONG with the received data
                        sendMessage(&net, send_sequence++, COMMAND_PONG, SessionID, data, received);
                        break;
                    }
                    
                    case COMMAND_PONG:
                    {
                        //TODO Verify ping response
                        break;
                    }
                    
                    case COMMAND_HELLO:
                    {
                        state = AUTHENTICATING;
                        //Server sends info on what it is capable of, version, etc
                        //uint32_t server version
                        //uint32_t protocol version
                        //uint32_t flags {
                        // 1 = need authentication
                        //}
                        
                        //if authentication, send COMMAND_AUTH with:
                        //uint8_t length
                        //uint8_t username[length]
                        //uint8_t length
                        //uint8_t password[length]
                        break;
                    }
                    
                    case COMMAND_AUTH:
                    {
                        //Server sends auth response
                        //uint8_t status
                        //uint8_t length
                        //uint8_t message[length]
                        
                        if(received < 2){
                            printf("Invalid length for COMMAND_AUTH header");
                            break;
                        }
                        
                        uint8_t status = data[0];
                        uint8_t length = data[1];
                        if(received < length + 2){
                            printf("Invalid length for COMMAND_AUTH message");
                            break;
                        }
                        
                        std::string message(data+2, data+2+length);
                        break;
                    }
                    
                    case COMMAND_SET_SESSIONID:
                    {
                        //This will be sent from a NULL session ID!
                        if(received == 16)
                            memcpy(SessionID, data, 16);
                        
                        sendMessage(&net, send_sequence++, COMMAND_SET_SESSIONID, SessionID, data, received);
                        
                        break;
                    }
                    
                    case COMMAND_MESSAGE:
                    {
                        //uint8_t type
                        //uint8_t length
                        //uint8_t message[length]
                        
                        if(received < 2){
                            printf("Invalid length for COMMAND_MESSAGE header");
                            break;
                        }
                        
                        uint8_t type = data[0];
                        uint8_t length = data[1];
                        if(received < length + 2){
                            printf("Invalid length for COMMAND_MESSAGE message");
                            break;
                        }
                        
                        std::string message(data+2, data+2+length);
                        
                        break;
                    }
                    
                    case COMMAND_FLIP:
                    {
                        //Flip a frame buffer
                        //uint8_t screen {
                        //  1 = top left
                        //  2 = top right
                        //  4 = bottom
                        //}
                        if(received < 1){
                            printf("Invalid length for COMMAND_FLIP");
                            break;
                        }
                        
                        bool flip_left = data[0] & 1 == 1;
                        bool flip_right = data[0] & 2 == 2;
                        bool flip_bottom = data[0] & 4 == 4;
                        
                        break;
                    }
                    
                    case COMMAND_FRAMEBUFFER:
                    {
                        //Write to frame buffer
                        //uint8_t flags;
                        //uint8_t count;
                        
                        if(received < 2){
                            printf("Invalid length for COMMAND_FRAMEBUFFER header");
                            break;
                        }
                        
                        int offset = 0;
                        uint8_t flags = data[offset]; offset++;
                        uint8_t count = data[offset]; offset++;
                        
                        for(int i = 0; i < count; i++){
                            //Repeatable for count or until out of readable memory
                            //In case of out of memory, raise COMMAND_ERROR.
                            //uint32_t flags {
                            // flags: 1111
                            // screen: 11
                            // x: 111111111
                            // y: 11111111
                            // length: 111111111
                            //}
                            //uint8_t data[length * 3];
                            if(offset + 4 > received)
                            {
                                printf("Invalid length for COMMAND_FRAMEBUFFER buffer flags");
                                break;
                            }
                            
                            int packedFlags = U8ToU32(data + offset); offset += 4;
                            
                            int length = packedFlags & 511;
                            int y = (packedFlags >> 9) & 255;
                            int x = (packedFlags >> 17) & 511;
                            int screen = (packedFlags >> 26) & 3;
                            int bufferFlags = (packedFlags >> 28) & 15;
                            
                            int readsize = length * 3;
                            if(offset + readsize > received)
                            {
                                printf("Invalid length for COMMAND_FRAMEBUFFER buffer data");
                                break;
                            }
                            
                            uint8_t image[readsize];
                            memcpy(image, data+offset, readsize);
                            
                            offset += readsize;
                        }
                        
                        break;
                    }
                    
                    case COMMAND_PCM:
                    {
                        //Write to audio
                        //uint8_t flags {
                        // 1 = mono
                        //}
                        //uint16_t length
                        
                        if(received < 3){
                            printf("Invalid length for COMMAND_PCM header");
                            break;
                        }
                        
                        int offset = 0;
                        
                        uint8_t flags = data[offset]; offset += 1;
                        bool mono = flags & 1;
                        
                        uint16_t length = U8ToU32(data + offset); offset += 2;
                        
                        uint16_t pcm_left[length];
                        uint16_t pcm_right[length];
                        
                        for(int i = 0; i < length; i++)
                        {
                            //uint16_t data[length * 2]
                            if(mono)
                            {
                                if(offset + 2 > received)
                                {
                                    printf("Invalid length for COMMAND_PCM data");
                                    break;
                                }
                                uint16_t value = U8ToS16(data + offset); offset += 2;
                                pcm_left[i] = pcm_right[i] = value;
                            }
                            else
                            {
                                if(offset + 4 > received)
                                {
                                    printf("Invalid length for COMMAND_PCM data");
                                    break;
                                }
                                // Left and right channel interlaced
                                pcm_left[i] = U8ToS16(data + offset); offset += 2;
                                pcm_right[i] = U8ToS16(data + offset); offset += 2;
                            }
                        }
                        
                        break;
                    }
                    
                    case COMMAND_CAMERA:
                    {
                        //Request camera frame
                        //All cameras selected will be taken at the same time
                        //uint8_t flags {
                        // 1 = rear left
                        // 2 = rear right
                        // 4 = front
                        //}
                        //uint32_t request_id //To tie frames back to each other
                        
                        break;
                    }
                    
                    case COMMAND_OPTIONS:
                    {
                        //Set options
                        //uint8_t bitmask{
                        // 1 = screen wide
                        // 2 = enable audio
                        //}
                        break;
                    }
                    
                    case COMMAND_ERROR:
                    {
                        //Server error. Client uses same format.
                        //uint8_t reason
                        //uint8_t length
                        //uint8_t textreason[length]
                        if(received < 2){
                            printf("Invalid length for COMMAND_ERROR header");
                            break;
                        }
                        
                        uint8_t reason = data[0];
                        uint8_t length = data[1];
                        if(received < length + 2){
                            printf("Invalid length for COMMAND_ERROR message");
                            break;
                        }
                        
                        std::string message(data+2, data+2+length);
                        break;
                    }
                        
                    case COMMAND_GOODBYE:
                    {
                        //Server disconnecting with reason
                        //uint8_t reason
                        //uint8_t length
                        //uint8_t textreason[length]
                        if(received < 2){
                            printf("Invalid length for COMMAND_GOODBYE header");
                            break;
                        }
                        
                        uint8_t reason = data[0];
                        uint8_t length = data[1];
                        if(received < length + 2){
                            printf("Invalid length for COMMAND_GOODBYE message");
                            break;
                        }
                        
                        std::string message(data+2, data+2+length);
                        break;
                    }
                    
                    default:
                        printf("Unknown opcode %i", buffer[0]);
                        continue;
                }
            }
        }
        
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    // Exit services
    gfxExit();
    return 0;
}