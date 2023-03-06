#include <SDL/SDL.h>
#include <3ds.h>
#include "common.h"

//Default images
#include "left_bgr.h"
#include "right_bgr.h"
#include "no_video_bgr.h"
#include "disconnected_bgr.h"
/*
//Nuklear
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_SDL_IMPLEMENTATION
#include "nuklear/nuklear.h"
#include "nuklear_sdl_12.h"
*/
#include "render.h"

bool supports3D()
{
    return true; //Something is wrong here
    u8 *model = 0;
    CFGU_GetSystemModel(model);
    switch(*model)
    {
        case CFG_MODEL_3DS:
        case CFG_MODEL_3DSXL:
        case CFG_MODEL_N3DS:
        case CFG_MODEL_N3DSXL:
            return true;
        case CFG_MODEL_2DS:
        case CFG_MODEL_N2DSXL:
            return false;
        default:
            failExit("Failed to identify system model");
            return false;
    }
}

bool supportsWide()
{
    return true; //Something is wrong here
    u8 *model = 0;
    CFGU_GetSystemModel(model);
    switch(*model)
    {
        case CFG_MODEL_3DS:
        case CFG_MODEL_3DSXL:
        case CFG_MODEL_N3DS:
        case CFG_MODEL_N3DSXL:
        case CFG_MODEL_N2DSXL:
            return true;
        case CFG_MODEL_2DS:
            return false;
        default:
            failExit("Failed to identify system model");
            return false;
    }
}

Renderer::Renderer()
{
    gfxSetScreenFormat(GFX_TOP, GSP_BGR8_OES);
    gfxSetScreenFormat(GFX_BOTTOM, GSP_BGR8_OES);
    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetDoubleBuffering(GFX_BOTTOM, true);
    EnableWide(supportsWide());
    Enable3D(supports3D());
    Reset();
    Flip();
}

Renderer::~Renderer()
{
    if(mScreenTopLeft != nullptr)
    {
        free(mScreenTopLeft);
        free(mScreenTopRight);
        free(mScreenBottom);
        free(mScreenTopLeftReady);
        free(mScreenTopRightReady);
        free(mScreenBottomReady);
    }
}

bool Renderer::EnableWide(bool enable)
{
    if(!supportsWide())
        enable = false;
    gfxSetWide(enable);
    mGraphicsWide = enable;
    Allocate();
    return enable;
}

bool Renderer::Enable3D(bool enable)
{
    if(!supports3D())
        enable = false;
    gfxSet3D(enable);
    mGraphics3D = enable;
    Allocate();
    return enable;
}


void Renderer::Render()
{
    u8* fbtl = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &mTopWidth, &mTopHeight);
    u8* fbtr = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);
    u8* fbb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &mBottomWidth, &mBottomHeight);
    
    memcpy(fbtl, mScreenTopLeftReady, mTopWidth * mTopHeight * 3);
    memcpy(fbtr, mScreenTopRightReady, mTopWidth * mTopHeight * 3);
    memcpy(fbb, mScreenBottomReady, mBottomWidth * mBottomHeight * 3);
    
    //If you want to do a overlay, do it here on fbtl, fbtr, and fbb.
    
    gfxFlushBuffers();
    gfxSwapBuffers();
}

void Renderer::Flip(bool top, bool bottom)
{
    if(top)
    {
        memcpy(mScreenTopLeftReady, mScreenTopLeft, mTopWidth * mTopHeight * 3);
        memcpy(mScreenTopRightReady, mScreenTopRight, mTopWidth * mTopHeight * 3);
    }
    if(bottom)
    {
        memcpy(mScreenBottomReady, mScreenBottom, mBottomWidth * mBottomHeight * 3);
    }
}

void Renderer::Reset()
{
    if(mGraphicsWide)
    {
        memcpy(mScreenTopLeft, no_video_bgr, no_video_bgr_size);
        memcpy(mScreenTopRight, no_video_bgr, no_video_bgr_size);
    }
    memcpy(mScreenBottom, disconnected_bgr, disconnected_bgr_size);
}

void Renderer::Allocate()
{
    if(mScreenTopLeft != nullptr)
    {
        free(mScreenTopLeft);
        free(mScreenTopRight);
        free(mScreenBottom);
        free(mScreenTopLeftReady);
        free(mScreenTopRightReady);
        free(mScreenBottomReady);
    }
    
    gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &mTopWidth, &mTopHeight);
    gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &mBottomWidth, &mBottomHeight);
    
    mScreenTopLeft = (uint8_t*)malloc(mTopWidth * mTopHeight * 3);
    mScreenTopRight = (uint8_t*)malloc(mTopWidth * mTopHeight * 3);
    mScreenBottom = (uint8_t*)malloc(mBottomWidth * mBottomHeight * 3);
    mScreenTopLeftReady = (uint8_t*)malloc(mTopWidth * mTopHeight * 3);
    mScreenTopRightReady = (uint8_t*)malloc(mTopWidth * mTopHeight * 3);
    mScreenBottomReady = (uint8_t*)malloc(mBottomWidth * mBottomHeight * 3);
}
