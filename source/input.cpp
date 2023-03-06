#include <3ds.h>
#include "common.h"
#include "input.h"

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

void packControls(InputMap_t *inputCurrent, uint8_t *p)
{
    u32ToU8(inputCurrent->held, p); p += 4;
    u32ToU8(inputCurrent->down, p); p += 4;
    u32ToU8(inputCurrent->up, p); p += 4;
    u16ToU8(inputCurrent->touchX, p); p += 2;
    u16ToU8(inputCurrent->touchY, p); p += 2;
    s16ToU8(inputCurrent->circleX, p); p += 2;
    s16ToU8(inputCurrent->circleY, p); p += 2;
    s16ToU8(inputCurrent->accelX, p); p += 2;
    s16ToU8(inputCurrent->accelY, p); p += 2;
    s16ToU8(inputCurrent->accelZ, p); p += 2;
    s16ToU8(inputCurrent->angleX, p); p += 2;
    s16ToU8(inputCurrent->angleY, p); p += 2;
    s16ToU8(inputCurrent->angleZ, p); p += 2;
    floatToU8(osGet3DSliderState(), p);
}