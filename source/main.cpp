#include <3ds.h>
#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_SDL_IMPLEMENTATION
#include "nuklear/nuklear.h"
#include "nuklear_sdl_12.h"

//This include a header containing definitions of our image
#include "left_bgr.h"
#include "right_bgr.h"
#include "no_video_bgr.h"
#include "disconnected_bgr.h"

#define MAX_MEMORY 0xffff

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

//---------------------------------------------------------------------------------
void failExit(const char *fmt, ...) {
//---------------------------------------------------------------------------------

	consoleInit(GFX_TOP, NULL);
	va_list ap;

	printf(CONSOLE_RED);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf(CONSOLE_RESET);
	printf("\nPress B to exit\n");

	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_B) exit(0);
	}
}

static u32 *SOC_buffer = NULL;

struct InputMap_t {
    u32 held;
    u32 down;
    u32 up;
    u16 touchX;
    u16 touchY;
    s16 circleX;
    s16 circleY;
    s16 accelX;
    s16 accelY;
    s16 accelZ;
    s16 angleX;
    s16 angleY;
    s16 angleZ;
};

void getInputs(InputMap_t *dest){
    hidScanInput();
    
    dest->held = hidKeysHeld();
    dest->down = hidKeysDown();
    dest->up = hidKeysUp();
    
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    dest->touchX = touchPos.px;
    dest->touchY = touchPos.py;
    
    circlePosition circlePos;
    hidCircleRead(&circlePos);
    dest->circleX = circlePos.dx;
    dest->circleY = circlePos.dy;
    
    accelVector accel;
    hidAccelRead(&accel);
    dest->accelX = accel.x;
    dest->accelY = accel.y;
    dest->accelZ = accel.z;
    
    angularRate angle;
    hidGyroRead(&angle);
    dest->angleX = angle.x;
    dest->angleY = angle.y;
    dest->angleZ = angle.z;
}

bool shouldSendControls(InputMap_t *a, InputMap_t *b){
    if(a->held != b->held)return true;
    if(a->down != b->down)return true;
    if(a->up != b->up)return true;
    if(a->touchX != b->touchX)return true;
    if(a->touchY != b->touchY)return true;
    if(a->circleX != b->circleX)return true;
    if(a->circleY != b->circleY)return true;
    if(a->accelX != b->accelX)return true;
    if(a->accelY != b->accelY)return true;
    if(a->accelZ != b->accelZ)return true;
    if(a->angleX != b->angleX)return true;
    if(a->angleY != b->angleY)return true;
    if(a->angleZ != b->angleZ)return true;
    return false;
}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define rmask 0xff0000
#define gmask 0xff00
#define bmask 0xff
#else
#define rmask 0xff
#define gmask 0xff00
#define bmask 0xff0000
#endif

void u32ToChar(unsigned long int const value, char * const buffer){
    buffer[0] = (value >> 24) & 0xFF;
    buffer[1] = (value >> 16) & 0xFF;
    buffer[2] = (value >> 8) & 0xFF;
    buffer[3] = value & 0xFF;
}

void s32ToChar(signed long int const value, char * const buffer){
    u32ToChar((unsigned long int)value, buffer);
}

void floatToChar(const float value, char * const buffer){
    char *res = ( char* )&value;
    buffer[0] = res[0];
    buffer[1] = res[1];
    buffer[2] = res[2];
    buffer[3] = res[3];
}

void u16ToChar(unsigned short const value, char * const buffer){
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] = value & 0xFF;
}
void s16ToChar(signed short int const value, char * const buffer){
    u16ToChar((unsigned short int)value, buffer);
}

unsigned long int charToU32(u8 *buffer){
    return (((unsigned char) buffer[0]) << 24)
          |(((unsigned char) buffer[1]) << 16)
          |(((unsigned char) buffer[2]) << 8)
          |(((unsigned char) buffer[3]));
}
signed long int charToS32(u8 *buffer){
    return (signed long int)charToU32(buffer);
}

unsigned short int charToU16(char const * const buffer){
    return ((unsigned char) buffer[0] << 8)
          |((unsigned char) buffer[1]);
}
signed short int charToS16(char const * const buffer){
    return (signed short int)charToU16(buffer);
}

bool showMenu = false;
static SDL_Surface *screen_surface;
struct nk_color background;
struct nk_colorf newBackground;
struct nk_context *ctx;
float bg[4];


NK_API nk_flags
nk_3ds_edit_string(struct nk_context *ctx, nk_flags flags,
    char *memory, int len, SwkbdType type, SwkbdValidInput filter){
    nk_flags event = nk_edit_string_zero_terminated(ctx, flags, memory, len, 0);
    if (event & NK_EDIT_ACTIVATED) {
        static SwkbdState swkbd;
        swkbdInit(&swkbd, type, 2, len);
        swkbdSetValidation(&swkbd, filter, 0, 0);
        swkbdSetFeatures(&swkbd, SWKBD_DARKEN_TOP_SCREEN);
        swkbdSetInitialText(&swkbd, memory);
        SwkbdButton button = swkbdInputText(&swkbd, memory, len);
        ctx->current->edit.active = nk_false;
    }
    return event;
}

static char connection_host[128] = "192.168.0.192";
static char connection_port[16] = "8888";
static char connection_user[256];
static char connection_pass[256];

static char statusText[256] = "Made with love by Felix";
enum state_t {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
    SHUTDOWN
};

state_t state = DISCONNECTED;
state_t targetState = DISCONNECTED;

void setStatus(char *msg){
    sprintf(statusText, msg);
    usleep(10);
}

int sockfd;
void networkConnect(){
    state = CONNECTING;
    Result ret=0;

    struct addrinfo hints;
    struct addrinfo *resaddr = NULL, *resaddr_cur;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd==-1)
    {
        failExit("Failed to create the socket.");
        state = DISCONNECTED;
        return;
    }
    printf("Socket created.\n");
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    printf("Resolving hostname...\n");

    if(getaddrinfo(connection_host, connection_port, &hints, &resaddr)!=0)
    {
        setStatus("getaddrinfo() failed.");
        closesocket(sockfd);
        state = DISCONNECTED;
        return;
    }

    printf("Connecting to the server...\n");
    
    for(resaddr_cur = resaddr; resaddr_cur != NULL; resaddr_cur = resaddr_cur->ai_next)
    {
        if(connect(sockfd, resaddr_cur->ai_addr, resaddr_cur->ai_addrlen)==0)break;
    }
    

    freeaddrinfo(resaddr);
    
    if(resaddr_cur==NULL)
    {
        setStatus("Failed to connect.");
        closesocket(sockfd);
        state = DISCONNECTED;
        return;
    }
    
    printf("Connected.\n");
    
    if(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1){
        failExit("Failed to configure sock.");
        closesocket(sockfd);
        state = DISCONNECTED;
        return;
    }
    printf("Set to non-blocking.\n");
    
    setStatus("Connected!");
    state = CONNECTED;
}

void networkDisconnect(){
    closesocket(sockfd);
    state = DISCONNECTED;
}

void renderUI(){
   /* Input */
    SDL_Event evt;
    nk_input_begin(ctx);
    while (SDL_PollEvent(&evt)) {
        nk_sdl_handle_event(&evt);
    }
    nk_input_end(ctx);

    {
        struct nk_panel layout;
        if (nk_begin(ctx, "if u see this u r stinky", nk_rect(0, 0, 320, 240), 0))
        {
            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
            nk_layout_row_push(ctx, 40);
            nk_label(ctx, "IP:", NK_TEXT_LEFT);
            nk_layout_row_push(ctx, 240);
            nk_3ds_edit_string(ctx, NK_EDIT_SIMPLE, connection_host, 128, SWKBD_TYPE_NORMAL, SWKBD_ANYTHING);
            nk_layout_row_end(ctx);
            
            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
            nk_layout_row_push(ctx, 40);
            nk_label(ctx, "Port:", NK_TEXT_LEFT);
            nk_layout_row_push(ctx, 240);
            nk_3ds_edit_string(ctx, NK_EDIT_SIMPLE, connection_port, 16, SWKBD_TYPE_NUMPAD, SWKBD_ANYTHING);
            nk_layout_row_end(ctx);
            
            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
            nk_layout_row_push(ctx, 40);
            nk_label(ctx, "User:", NK_TEXT_LEFT);
            nk_layout_row_push(ctx, 240);
            nk_3ds_edit_string(ctx, NK_EDIT_SIMPLE, connection_user, 128, SWKBD_TYPE_NORMAL, SWKBD_ANYTHING);
            nk_layout_row_end(ctx);
            
            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
            nk_layout_row_push(ctx, 40);
            nk_label(ctx, "Pass:", NK_TEXT_LEFT);
            nk_layout_row_push(ctx, 240);
            nk_3ds_edit_string(ctx, NK_EDIT_SIMPLE, connection_pass, 128, SWKBD_TYPE_NUMPAD, SWKBD_ANYTHING);
            nk_layout_row_end(ctx);
            
            nk_layout_row_dynamic(ctx, 25, 1);
            static int check = nk_true;
            nk_checkbox_label(ctx, "Remember password", &check);
            
            nk_layout_row_dynamic(ctx, 25, 1);
            nk_label(ctx, statusText, NK_TEXT_CENTERED);
            nk_layout_row_dynamic(ctx, 25, 2);
            if(state == DISCONNECTED){
                if (nk_button_label(ctx, "Connect")){
                    setStatus("Connecting...");
                    networkConnect();
                }
            }else if(state == CONNECTED){
                if (nk_button_label(ctx, "Disconnect")){
                    setStatus("Disconnecting...");
                    networkDisconnect();
                }
            }else{
                nk_button_label(ctx, "...");
            }
            
            if (nk_button_label(ctx, "Close Menu"))
                showMenu = false;
        }
        nk_end(ctx);
    }
    
    nk_color_fv(bg, background);
    nk_sdl_render(nk_rgb(background.r, background.g, background.b));
}

int readIntoBuffer(int fd, u8 *buffer, int dSize){
    int i = 0;
    int s;
    while(i < dSize){
        int size = 1024;
        if(dSize - i < size){
            size = dSize - i;
            s = recv(fd, buffer + i, size - 2, 0);
            i += s + 1;
        }else{
            s = recv(fd, buffer + i, size, 0);
            i += s;
        }
        if(s < 0){
            return 1;
        }
    }
    return 0;
}


int main(int argc, char **argv){
    int ret;
    gfxInitDefault();
    
    gfxSetWide(true);
    gfxSet3D(true);
    
    // allocate buffer for SOC service
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

    if(SOC_buffer == NULL) {
        failExit("memalign: failed to allocate\n");
    }

    // Now intialise soc:u service
    if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
        failExit("socInit: 0x%08X\n", (unsigned int)ret);
    }
    
    
    HIDUSER_EnableAccelerometer();
    HIDUSER_EnableGyroscope();

    /* SDL setup */
    if( SDL_Init(SDL_INIT_VIDEO) == -1) {
        failExit( "Can't init SDL:  %s\n", SDL_GetError( ) );
        return 1;
    }
    
    screen_surface = SDL_SetVideoMode(320, 240, 24, SDL_BOTTOMSCR);
    
    if(screen_surface == NULL) {
        failExit( "Can't set video mode: %s\n", SDL_GetError( ) );
        return 1;
    }
    
    s32 prio = 0;
    
    ctx = nk_sdl_init(screen_surface);
    background = nk_rgb(28, 48, 62);

    gfxSetDoubleBuffering(GFX_TOP, false);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);

    u8* fbtl = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    u8* fbtr = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);
    u8* fbb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    u8 *megabuf = (u8*)malloc(0xFFFFF);
    InputMap_t inputPrevious;
    InputMap_t inputCurrent;
    while (aptMainLoop()){
        //Wait for VBlank
        gspWaitForVBlank();
        
        getInputs(&inputCurrent);
        
        if(state == SHUTDOWN)
            break;
        
        if(state == CONNECTED){
            if(shouldSendControls(&inputCurrent, &inputPrevious)){
                char arr[37];
                char *p; p = (char*)arr;
                p[0] = 8; p++;
                u32ToChar(inputCurrent.held, p); p += 4;
                u32ToChar(inputCurrent.down, p); p += 4;
                u32ToChar(inputCurrent.up, p); p += 4;
                u16ToChar(inputCurrent.touchX, p); p += 2;
                u16ToChar(inputCurrent.touchY, p); p += 2;
                s16ToChar(inputCurrent.circleX, p); p += 2;
                s16ToChar(inputCurrent.circleY, p); p += 2;
                s16ToChar(inputCurrent.accelX, p); p += 2;
                s16ToChar(inputCurrent.accelY, p); p += 2;
                s16ToChar(inputCurrent.accelZ, p); p += 2;
                s16ToChar(inputCurrent.angleX, p); p += 2;
                s16ToChar(inputCurrent.angleY, p); p += 2;
                s16ToChar(inputCurrent.angleZ, p); p += 2;
                floatToChar(osGet3DSliderState(), p);
                send(sockfd, (char*)arr, sizeof(arr), 0);
            }
            send(sockfd, "\x03", 1, 0); //Request frame
            char op[1];
            int ss = 1;
            while(ss > 0){
                ss = recv(sockfd, op, sizeof(op), 0);
                //setStatus("data get!");
                switch(op[0]){
                    case 0x04:{
                        setStatus("Frame get!");
                        u8 bufOps[6];
                        if(readIntoBuffer(sockfd, bufOps, 6)){
                            setStatus("Corrupted stream!");
                            networkDisconnect();
                            break;
                        }
                        int bufId = bufOps[0];
                        int flags = bufOps[1] >> 4;
                        int type = bufOps[1] & 0xF;
                        int dSize = charToU32(bufOps + 1);
                        if(dSize > 0xFFFFFF){
                            setStatus("Big buffer!");
                            sprintf(statusText, "%i", dSize);
                            networkDisconnect();
                            break;
                        }
                        sprintf(statusText, "%i %i %i %i %i", bufId, flags, type, dSize);
                        if(type == 0){
                            if(bufId == 0 || bufId == 1 || bufId == 2){
                                u8 *bufPointer;
                                if(bufId == 0)
                                    bufPointer = fbtl;
                                else if(bufId == 1)
                                    bufPointer = fbtr;
                                else if(bufId == 2)
                                    bufPointer = fbb;
                                if(readIntoBuffer(sockfd, bufPointer, dSize)){
                                    setStatus("Error!");
                                    networkDisconnect();
                                    break;
                                }
                            }else{
                                setStatus("Invalid buffer!");
                                networkDisconnect();
                            }
                            break;
                        }
                    };break;
                    default:
                        sprintf(statusText, "bad op %i", op[0]);
                        networkDisconnect();
                }
            }
        }
        
        if(inputCurrent.down & KEY_START)
            showMenu = !showMenu;
        
        if(showMenu){
            renderUI();
        }else if(state != CONNECTED){
            memcpy(fbb, disconnected_bgr, disconnected_bgr_size);
        }
        
        if(state != CONNECTED){
            memcpy(fbtl, no_video_bgr, no_video_bgr_size);
            memcpy(fbtr, no_video_bgr, no_video_bgr_size);
        }
        
        // Flush and swap framebuffers
        gfxFlushBuffers();
        gfxSwapBuffers();

        inputPrevious = inputCurrent;
    }
    state = SHUTDOWN;

    socExit();
    nk_sdl_shutdown();
    SDL_Quit();
    gfxExit();
    return 0;
}