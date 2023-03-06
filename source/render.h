#ifndef RENDER_H_
#define RENDER_H_
#include <3ds.h>

class Renderer
{
public:
    enum BottomScreenMode_t {
        SCREEN_BUFFER = 0,
        SCREEN_MENU = 1
    };
    BottomScreenMode_t mBottomScreenMode = SCREEN_BUFFER;
    bool mGraphicsWide = false;
    bool mGraphics3D = false;
    uint8_t *mScreenTopLeft;
    uint8_t *mScreenTopRight;
    uint8_t *mScreenBottom;

    u16 mTopWidth = 0;
    u16 mTopHeight = 0;
    u16 mBottomWidth = 0;
    u16 mBottomHeight = 0;

private:
    uint8_t *mScreenTopLeftReady;
    uint8_t *mScreenTopRightReady;
    uint8_t *mScreenBottomReady;
    void Allocate();


public:
    Renderer();
    ~Renderer();
    bool Enable3D(bool enable);
    bool EnableWide(bool enable);
    void Reset();
    void Render();
    void Flip(){
        Flip(true, true);
    }
    void Flip(bool top, bool bottom);
};

#endif 
