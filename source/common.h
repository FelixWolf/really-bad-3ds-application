#ifndef COMMON_H_
#define COMMON_H_
#include <3ds.h>
#include <sys/types.h>

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define rmask 0xff0000
#define gmask 0xff00
#define bmask 0xff
#else
#define rmask 0xff
#define gmask 0xff00
#define bmask 0xff0000
#endif

void u32ToU8(unsigned long int const value, uint8_t * const buffer);
void s32ToU8(signed long int const value, uint8_t * const buffer);
void floatToU8(const float value, uint8_t * const buffer);
void u16ToU8(unsigned short const value, uint8_t * const buffer);
void s16ToU8(signed short int const value, uint8_t * const buffer);
unsigned long int U8ToU32(u8 *buffer);
signed long int U8ToS32(u8 *buffer);
unsigned short int U8ToU16(uint8_t const * const buffer);
signed short int U8ToS16(uint8_t const * const buffer);

void failExit(const char *fmt, ...);

#endif