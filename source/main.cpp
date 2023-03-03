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

void sendMessage(Network *net, uint8_t command, uint8_t session[16], uint8_t *data, int datalen){
    uint8_t result[17 + datalen];
    result[0] = command;
    memcpy(result+1, session, 16);
    memcpy(result+17, data, datalen);
    net->send(result, sizeof(result));
}

#define COMMAND_PING 0
#define COMMAND_PONG 1
#define COMMAND_HELLO 2
#define COMMAND_AUTH 3
#define COMMAND_SET_SESSIONID 4
#define COMMAND_GOODBYE 255
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
    uint8_t SessionID[16];
    // Main loop
    while (aptMainLoop())
    {
        getInputs(&inputCurrent);
        //Print the CirclePad position
        //printf("Waiting...\n");
        if(net.connected()){
            if(state == CONNECTED && shouldSendControls(&inputCurrent, &inputPrevious)){
                uint8_t *arr[CONTROL_PACK_SIZE];
                packControls(&inputCurrent, arr);
                net.send((uint8_t*)arr, sizeof(arr));
            }
            while(true){
                uint8_t buffer[512];
                int received = net.recv(buffer, sizeof(buffer));
                if(received <= 0)
                    break; //No data? No more messages
                
                if(memcmp(buffer+1, SessionID, 16) != 0)
                    continue; //Not for us
                
                uint8_t *data = buffer+17;
                
                received -= 17;
                
                switch(buffer[0])
                {
                    case COMMAND_PING:
                        sendMessage(&net, COMMAND_PONG, SessionID, data, received);
                        break;
                    
                    case COMMAND_PONG:
                        //TODO Verify ping response
                        break;
                    
                    case COMMAND_HELLO:
                        //Server sends info on what it is capable of, version, etc
                        break;
                    
                    case COMMAND_AUTH:
                        //Server sends auth response
                        break;
                    
                    case COMMAND_SET_SESSIONID:
                        //This will be sent from a NULL session ID!
                        if(received == 16)
                            memcpy(SessionID, data, 16);
                        break;
                    
                    case COMMAND_GOODBYE:
                        //Server disconnecting with reason
                        break;
                    
                    default:
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