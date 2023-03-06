#ifndef INPUT_H_
#define INPUT_H_
#define CONTROL_PACK_SIZE 36

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

void getInputs(InputMap_t *dest);
bool shouldSendControls(InputMap_t *a, InputMap_t *b);

void packControls(InputMap_t *inputCurrent, uint8_t *p);
#endif